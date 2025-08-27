#include "view.hpp"

#include <cstring>
#include <expected>
#include <format>
#include <string_view>
#include <type_traits>

#include <re2/re2.h>

#include "core/assert.hpp"
#include "core/context.hpp"
#include "core/thread.hpp"
#include "utils/format.hpp"
#include "utils/memory.hpp"
#include "utils/time.hpp"
#include "utils/units.hpp"

namespace core
{

enum struct Type : char
{
    uninitialized,
    base,
    filtered,
};

enum struct State : char
{
    uninitialized,
    loading,
    loaded,
    aborted,
};

constexpr static size_t BLOCK_SIZE = 16_MiB;

template <typename T>
constexpr static inline char cast(T value)
{
    return static_cast<char>(value);
}

View::View()
    : stopFlag_(false)
    , state_(cast(State::uninitialized))
    , type_(cast(Type::uninitialized))
    , lineCount_(0)
    , fileLines_(nullptr)
{
}

View::~View()
{
    assert(isMainThread(), "~View called not on main thread");
    switch (state_)
    {
        case cast(State::loading):
            assert(stopFlag_ == false, "Stop flag was already set to true");
            stopFlag_ = true;
            while (state_ == cast(State::loading));
            break;
        default:
            break;
    }
    switch (type_)
    {
        case cast(Type::base):
            utils::destroyAt(&ownLines_);
            break;
        case cast(Type::filtered):
            utils::destroyAt(&greppedLines_);
            break;
        default:
            break;
    }
}

template <typename T>
constexpr static inline const char* stringify(char value)
{
    if constexpr (std::is_same_v<T, State>)
    {
#define PRINT_STATE(STATE) \
    case State::STATE: return #STATE
        switch (static_cast<State>(value))
        {
            PRINT_STATE(uninitialized);
            PRINT_STATE(loading);
            PRINT_STATE(loaded);
            PRINT_STATE(aborted);
        }
        return "unknown";
    }
    else if constexpr (std::is_same_v<T, Type>)
    {
#define PRINT_TYPE(TYPE) \
    case Type::TYPE: return #TYPE
        switch (static_cast<Type>(value))
        {
            PRINT_TYPE(uninitialized);
            PRINT_TYPE(base);
            PRINT_TYPE(filtered);
        }
        return "unknown";
    }
    else
    {
        static_assert(0, "Stringify not implemented for type");
    }
}

void View::load(std::string path, Context&, ViewLoadedCallback callback)
{
    loading();

    if (auto result = file_.open(std::move(path)); not result) [[unlikely]]
    {
        state_ = cast(State::aborted);
        callback(std::unexpected(std::move(result.error())));
        return;
    }

    utils::constructAt(&ownLines_);
    type_ = cast(Type::base);

    async(
        [callback = std::move(callback), this]
        {
            auto timer = utils::startTimeMeasurement();

            auto result = loadFile();

            if (result) [[likely]]
            {
                state_ = cast(State::loaded);
                callback(timer.elapsed());
            }
            else
            {
                state_ = cast(State::aborted);
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void View::grep(std::string pattern, GrepOptions options, ViewId parentViewId, Context& context, ViewLoadedCallback callback)
{
    loading();

    utils::constructAt(&greppedLines_);
    type_ = cast(Type::filtered);

    async(
        [pattern = std::move(pattern), callback = std::move(callback), options, parentViewId, &context, this]
        {
            auto parentView = getView(parentViewId, context);

            if (not parentView) [[unlikely]]
            {
                state_ = cast(State::aborted);
                callback(std::unexpected("Parent view has been closed"));
                return;
            }

            copyFromParent(*parentView);

            auto timer = utils::startTimeMeasurement();

            auto result = grep(std::move(pattern), options, *parentView);

            if (result) [[likely]]
            {
                state_ = cast(State::loaded);
                callback(timer.elapsed());
            }
            else
            {
                state_ = cast(State::aborted);
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void View::filter(size_t start, size_t end, ViewId parentViewId, Context& context, ViewLoadedCallback callback)
{
    loading();

    utils::constructAt(&greppedLines_);
    type_ = cast(Type::filtered);

    async(
        [start, end, callback = std::move(callback), parentViewId, &context, this]
        {
            auto parentView = getView(parentViewId, context);

            if (not parentView) [[unlikely]]
            {
                state_ = cast(State::aborted);
                callback(std::unexpected("Parent view has been closed"));
                return;
            }

            copyFromParent(*parentView);

            auto timer = utils::startTimeMeasurement();

            filter(start, end, *parentView);

            state_ = cast(State::loaded);
            callback(timer.elapsed());
        });
}

StringOrError View::readLine(size_t i)
{
    auto lineIndex = i;

    if (type_ == cast(Type::filtered))
    {
        lineIndex = greppedLines_[lineIndex];
    }

    auto& line = (*fileLines_)[lineIndex];

    if (line.len == 0)
    {
        return "";
    }

    auto result = readInternal(line);

    if (not result) [[unlikely]]
    {
        return std::unexpected(std::move(result.error()));
    }

    return std::string(result.value());
}

size_t View::absoluteLineNumber(size_t lineIndex) const
{
    switch (type_)
    {
        case cast(Type::filtered):
            return greppedLines_[lineIndex];
        default:
            return lineIndex;
    }
}

size_t View::fileLineCount() const
{
    return fileLines_->size();
}

const std::string& View::filePath() const
{
    assert(type_ != cast(Type::uninitialized), std::format("View {} type is uninitialized", this));
    return file_.path();
}

void View::loading()
{
    assert(state_ == cast(State::uninitialized), std::format("View {} state is {}", this, stringify<State>(state_)));
    assert(type_ == cast(Type::uninitialized), std::format("View {} type is {}", this, stringify<Type>(type_)));

    state_ = cast(State::loading);
}

void View::copyFromParent(View& parentView)
{
    file_.clone(parentView.file_);
    fileLines_ = parentView.fileLines_;
}

void View::initialize(Lines&& lines)
{
    lineCount_ = lines.size();
    ownLines_ = std::move(lines);
    fileLines_ = &ownLines_;
}

void View::initialize(LineRefs&& lines)
{
    lineCount_ = lines.size();
    greppedLines_ = std::move(lines);
}

std::expected<bool, std::string> View::loadFile()
{
    Lines lines;
    auto sizeLeft{file_.size()};
    size_t offset{0};
    size_t lineStart{0};

    while (sizeLeft)
    {
        auto toRead = std::min(sizeLeft, BLOCK_SIZE);

        if (auto result = file_.remap(offset, toRead); not result) [[unlikely]]
        {
            return std::unexpected(std::move(result.error()));
        }

        const auto text = file_.at(offset);

        for (size_t i = 0; i < toRead; ++i)
        {
            if (text[i] == '\n')
            {
                if (stopFlag_) [[unlikely]]
                {
                    return std::unexpected("Loading was aborted");
                }

                lines.emplace_back(Line{.start = lineStart, .len = offset + i - lineStart});
                lineStart = offset + i + 1;
            }
        }

        sizeLeft -= toRead;
        offset += toRead;
    }

    initialize(std::move(lines));

    return true;
}

static bool caseSensitiveCheck(const std::string_view& line, const std::string& pattern)
{
    return line.contains(pattern);
}

static bool caseInsensitiveCheck(const std::string_view& line, const std::string& pattern)
{
    return std::search(
        line.begin(), line.end(),
        pattern.begin(), pattern.end(),
        [](char c1, char c2)
        {
            return std::tolower(c1) == std::tolower(c2);
        }) != line.end();
}

static RE2 regexCreate(std::string pattern, bool caseInsensitive)
{
    RE2::Options reOptions;
    reOptions.set_log_errors(false);
    reOptions.set_case_sensitive(not caseInsensitive);

    return RE2(std::move(pattern), reOptions);
}

std::expected<bool, std::string> View::grep(
    std::string pattern,
    GrepOptions options,
    View& parentView)
{
    LineRefs lines;

    #define FILE_LINE_INDEX_TRANSFORM(I) I
    #define FILTERED_LINE_INDEX_TRANSFORM(I) parentView.greppedLines_[I]

    #define GREP_LOOP(CONDITION, LINE_INDEX_TRANSFORM) \
        do \
        { \
            auto& fileLines = *fileLines_; \
            for (size_t i = 0; i < lineCount; ++i) \
            { \
                if (stopFlag_) [[unlikely]] \
                { \
                    return std::unexpected("Loading was aborted"); \
                } \
                auto lineIndex = LINE_INDEX_TRANSFORM(i); \
                auto result = readInternal(fileLines[lineIndex]); \
                if (not result) [[unlikely]] \
                { \
                    return std::unexpected(std::move(result.error())); \
                } \
                if (CONDITION) \
                { \
                    lines.emplace_back(lineIndex); \
                } \
            } \
        } \
        while (0)

    size_t lineCount = parentView.lineCount_;

    if (not options.regex)
    {
        auto lineCheck = options.caseInsensitive
            ? &caseInsensitiveCheck
            : &caseSensitiveCheck;

        if (options.inverted)
        {
            if (parentView.type_ == cast(Type::filtered))
            {
                GREP_LOOP(not lineCheck(result.value(), pattern), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(not lineCheck(result.value(), pattern), FILE_LINE_INDEX_TRANSFORM);
            }
        }
        else
        {
            if (parentView.type_ == cast(Type::filtered))
            {
                GREP_LOOP(lineCheck(result.value(), pattern), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(lineCheck(result.value(), pattern), FILE_LINE_INDEX_TRANSFORM);
            }
        }
    }
    else
    {
        auto re = regexCreate(std::move(pattern), options.caseInsensitive);

        if (options.inverted)
        {
            if (parentView.type_ == cast(Type::filtered))
            {
                GREP_LOOP(not RE2::PartialMatch(result.value(), re), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(not RE2::PartialMatch(result.value(), re), FILE_LINE_INDEX_TRANSFORM);
            }
        }
        else
        {
            if (parentView.type_ == cast(Type::filtered))
            {
                GREP_LOOP(RE2::PartialMatch(result.value(), re), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(RE2::PartialMatch(result.value(), re), FILE_LINE_INDEX_TRANSFORM);
            }
        }
    }

    initialize(std::move(lines));

    return true;
}

void View::filter(
    size_t start,
    size_t end,
    View& parentView)
{
    LineRefs lines;
    for (size_t i = start; i <= end; ++i)
    {
        auto lineIndex = i;

        if (parentView.type_ == cast(Type::filtered))
        {
            lineIndex = parentView.greppedLines_[lineIndex];
        }

        lines.push_back(lineIndex);
    }

    initialize(std::move(lines));
}

std::expected<std::string_view, std::string> View::readInternal(Line line)
{
    if (line.len == 0)
    {
        return "";
    }

    if (not file_.isAreaMapped(line.start, line.len)) [[unlikely]]
    {
        auto mappingLen = std::min(BLOCK_SIZE, file_.size() - line.start);

        if (auto result = file_.remap(line.start, mappingLen); not result) [[unlikely]]
        {
            return std::unexpected(std::move(result.error()));
        }
    }

    return std::string_view(file_.at(line.start), line.len);
}

}  // namespace core

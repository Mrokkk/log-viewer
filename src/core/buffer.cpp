#include "buffer.hpp"

#include <algorithm>
#include <cstring>
#include <expected>
#include <string_view>
#include <thread>
#include <type_traits>

#include <re2/re2.h>

#include "core/assert.hpp"
#include "core/config.hpp"
#include "core/context.hpp"
#include "core/thread.hpp"
#include "utils/format.hpp"
#include "utils/math.hpp"
#include "utils/memory.hpp"
#include "utils/time.hpp"
#include "utils/units.hpp"

namespace core
{

using MaybeError = std::expected<bool, std::string>;
using StringViewOrError = std::expected<std::string_view, std::string>;

struct Buffer::Impl final : Buffer
{
    Impl() = delete;

    constexpr static inline Impl& get(Buffer* b)
    {
        return *static_cast<Impl*>(b);
    }

    void busy();
    void copyFromParent(Buffer& parentBuffer);
    void initialize(Lines&& lines);
    void initialize(LineRefs&& lineRefs);

    MaybeError loadFile();

    MaybeError grep(
        std::string pattern,
        GrepOptions options,
        Buffer& parentBuffer);

    MaybeError parallelGrep(
        std::string pattern,
        GrepOptions options,
        Buffer& parentBuffer,
        Context& context);

    MaybeError grepInternal(
        std::string pattern,
        GrepOptions options,
        Buffer& parentBuffer,
        File& file,
        size_t start,
        size_t end,
        LineRefs& lineRefs);

    void filter(
        size_t start,
        size_t end,
        Buffer& parentBuffer);

    SearchResult search(
        const SearchRequest& req,
        File& file);

    StringViewOrError readInternal(Line line);
    StringViewOrError readInternal(Line line, File& file);
};

enum struct BufferType : char
{
    uninitialized,
    base,
    filtered,
};

enum struct State : char
{
    uninitialized,
    busy,
    idle,
    aborted,
};

constexpr static size_t BLOCK_SIZE = 16_MiB;

template <typename T>
constexpr static inline char cast(T value)
{
    return static_cast<char>(value);
}

Buffer::Buffer()
    : mStopFlag(false)
    , mState(cast(State::uninitialized))
    , mType(cast(BufferType::uninitialized))
    , mLineCount(0)
    , mFileLines(nullptr)
{
    static_assert(sizeof(Impl) == sizeof(Buffer));
}

Buffer::~Buffer()
{
    assert(isMainThread(), "~Buffer called not on main thread");
    switch (mState)
    {
        case cast(State::busy):
            mStopFlag = true;
            while (mState == cast(State::busy));
            break;
        default:
            break;
    }
    switch (mType)
    {
        case cast(BufferType::base):
            utils::destroyAt(&mOwnLines);
            break;
        case cast(BufferType::filtered):
            utils::destroyAt(&mFilteredLines);
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
            PRINT_STATE(busy);
            PRINT_STATE(idle);
            PRINT_STATE(aborted);
        }
        return "unknown";
    }
    else if constexpr (std::is_same_v<T, BufferType>)
    {
#define PRINT_TYPE(TYPE) \
    case BufferType::TYPE: return #TYPE
        switch (static_cast<BufferType>(value))
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

void Buffer::load(std::string path, Context&, FinishedCallback callback)
{
    auto& impl = Impl::get(this);

    impl.busy();

    if (auto result = mFile.open(std::move(path)); not result) [[unlikely]]
    {
        mState = cast(State::aborted);
        callback(std::unexpected(std::move(result.error())));
        return;
    }

    utils::constructAt(&mOwnLines);
    mType = cast(BufferType::base);

    async(
        [callback = std::move(callback), &impl]
        {
            auto timer = utils::startTimeMeasurement();

            auto result = impl.loadFile();

            if (result) [[likely]]
            {
                impl.mState = cast(State::idle);
                callback(timer.elapsed());
            }
            else
            {
                impl.mState = cast(State::aborted);
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void Buffer::grep(std::string pattern, GrepOptions options, BufferId parentBufferId, Context& context, FinishedCallback callback)
{
    auto& impl = Impl::get(this);

    impl.busy();

    utils::constructAt(&mFilteredLines);
    mType = cast(BufferType::filtered);

    async(
        [pattern = std::move(pattern), callback = std::move(callback), options, parentBufferId, &context, &impl]
        {
            auto parentBuffer = getBuffer(parentBufferId, context);

            if (not parentBuffer) [[unlikely]]
            {
                impl.mState = cast(State::aborted);
                callback(std::unexpected("Parent buffer has been closed"));
                return;
            }

            impl.copyFromParent(*parentBuffer);

            auto timer = utils::startTimeMeasurement();

            bool runParallel = parentBuffer->mLineCount > context.config.linesPerThread
                and context.config.maxThreads > 1;

            const auto result = runParallel
                ? impl.parallelGrep(std::move(pattern), options, *parentBuffer, context)
                : impl.grep(std::move(pattern), options, *parentBuffer);

            if (result) [[likely]]
            {
                impl.mState = cast(State::idle);
                callback(timer.elapsed());
            }
            else
            {
                impl.mState = cast(State::aborted);
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void Buffer::filter(size_t start, size_t end, BufferId parentBufferId, Context& context, FinishedCallback callback)
{
    auto& impl = Impl::get(this);

    impl.busy();

    utils::constructAt(&mFilteredLines);
    mType = cast(BufferType::filtered);

    async(
        [start, end, callback = std::move(callback), parentBufferId, &context, &impl]
        {
            auto parentBuffer = getBuffer(parentBufferId, context);

            if (not parentBuffer) [[unlikely]]
            {
                impl.mState = cast(State::aborted);
                callback(std::unexpected("Parent buffer has been closed"));
                return;
            }

            impl.copyFromParent(*parentBuffer);

            auto timer = utils::startTimeMeasurement();

            impl.filter(start, end, *parentBuffer);

            impl.mState = cast(State::idle);
            callback(timer.elapsed());
        });
}

StringOrError Buffer::readLine(size_t i)
{
    auto lineIndex = i;

    if (mType == cast(BufferType::filtered))
    {
        lineIndex = mFilteredLines[lineIndex];
    }

    auto& line = (*mFileLines)[lineIndex];

    if (line.len == 0)
    {
        return "";
    }

    auto result = Impl::get(this).readInternal(line);

    if (not result) [[unlikely]]
    {
        return std::unexpected(std::move(result.error()));
    }

    return std::string(result.value());
}

void Buffer::search(SearchRequest req, FinishedSearchCallback callback)
{
    if (mState == cast(State::busy))
    {
        mStopFlag = true;
        while (mState == cast(State::busy));
        mStopFlag = false;
    }

    mState = cast(State::busy);

    async(
        [callback = std::move(callback), req = std::move(req), this]
        {
            auto file = mFile;
            auto timer = utils::startTimeMeasurement();
            auto result = Impl::get(this).search(req, file);
            mState = cast(State::idle);
            callback(result, timer.elapsed());
        });
}

size_t Buffer::absoluteLineNumber(size_t lineIndex) const
{
    switch (mType)
    {
        case cast(BufferType::filtered):
            return mFilteredLines[lineIndex];
        default:
            return lineIndex;
    }
}

size_t Buffer::fileLineCount() const
{
    return mFileLines->size();
}

const std::string& Buffer::filePath() const
{
    assert(mType != cast(BufferType::uninitialized), utils::format("Buffer {} type is uninitialized", this));
    return mFile.path();
}

void Buffer::Impl::busy()
{
    assert(mState == cast(State::uninitialized), utils::format("Buffer {} state is {}", this, stringify<State>(mState)));
    assert(mType == cast(BufferType::uninitialized), utils::format("Buffer {} type is {}", this, stringify<BufferType>(mType)));

    mState = cast(State::busy);
}

void Buffer::Impl::copyFromParent(Buffer& parentBuffer)
{
    mFile = parentBuffer.mFile;
    mFileLines = parentBuffer.mFileLines;
}

void Buffer::Impl::initialize(Lines&& lines)
{
    mLineCount = lines.size();
    mOwnLines = std::move(lines);
    mFileLines = &mOwnLines;
}

void Buffer::Impl::initialize(LineRefs&& lines)
{
    mLineCount = lines.size();
    mFilteredLines = std::move(lines);
}

MaybeError Buffer::Impl::loadFile()
{
    Lines lines;
    auto sizeLeft{mFile.size()};
    size_t offset{0};
    size_t lineStart{0};

    while (sizeLeft)
    {
        auto toRead = utils::min(sizeLeft, BLOCK_SIZE);

        if (auto result = mFile.remap(offset, toRead); not result) [[unlikely]]
        {
            return std::unexpected(std::move(result.error()));
        }

        const auto text = mFile.at(offset);

        for (size_t i = 0; i < toRead; ++i)
        {
            if (text[i] == '\n')
            {
                if (mStopFlag) [[unlikely]]
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

MaybeError Buffer::Impl::grep(
    std::string pattern,
    GrepOptions options,
    Buffer& parentBuffer)
{
    LineRefs lines;

    auto result = grepInternal(
        std::move(pattern),
        options,
        parentBuffer,
        mFile,
        0,
        parentBuffer.lineCount(),
        lines);

    if (not result) [[unlikely]]
    {
        return result;
    }

    initialize(std::move(lines));

    return result;
}

MaybeError Buffer::Impl::parallelGrep(
    std::string pattern,
    GrepOptions options,
    Buffer& parentBuffer,
    Context& context)
{
    const auto lineCount = parentBuffer.mLineCount;
    const auto maxThreads = context.config.maxThreads;
    const auto linesPerThread = context.config.linesPerThread;

    const auto threadCount = utils::min(
        (lineCount + linesPerThread - 1) / linesPerThread,
        size_t(maxThreads));

    std::vector<LineRefs> lineRefsPerThread(threadCount);
    std::vector<std::thread> threads(threadCount);
    std::vector<MaybeError> results(threadCount);

    for (size_t i = 0; i < threadCount; ++i)
    {
        auto start = (lineCount / threadCount) * i;
        auto end = (lineCount / threadCount) * (i + 1);

        if (i == threadCount - 1)
        {
            end = lineCount;
        }

        auto& threadLines = lineRefsPerThread[i];
        auto& threadResult = results[i];
        auto threadFile = mFile;

        threads[i] = std::thread(
            [pattern, options, &parentBuffer, start, end,
                &threadLines, &threadResult,
                threadFile = std::move(threadFile),
                this] mutable
            {
                threadResult = grepInternal(
                    std::move(pattern),
                    options,
                    parentBuffer,
                    threadFile,
                    start,
                    end,
                    threadLines);
            });
    }

    MaybeError result(true);
    size_t greppedLineCount = 0;

    for (size_t i = 0; i < threadCount; ++i)
    {
        threads[i].join();
        if (not results[i]) [[unlikely]]
        {
            result = std::move(results[i]);
        }
        greppedLineCount += lineRefsPerThread[i].size();
    }

    if (not result)
    {
        return result;
    }

    LineRefs lines;
    lines.reserve(greppedLineCount);

    for (const auto& lineRefs : lineRefsPerThread)
    {
        std::copy(
            lineRefs.begin(), lineRefs.end(),
            std::back_inserter(lines));
    }

    initialize(std::move(lines));

    return true;
}

MaybeError Buffer::Impl::grepInternal(
    std::string pattern,
    GrepOptions options,
    Buffer& parentBuffer,
    File& file,
    size_t start,
    size_t end,
    LineRefs& lines)
{
    #define FILE_LINE_INDEX_TRANSFORM(I) I
    #define FILTERED_LINE_INDEX_TRANSFORM(I) parentBuffer.mFilteredLines[I]

    #define GREP_LOOP(CONDITION, LINE_INDEX_TRANSFORM) \
        do \
        { \
            auto& fileLines = *mFileLines; \
            for (size_t i = start; i < end; ++i) \
            { \
                if (mStopFlag) [[unlikely]] \
                { \
                    return std::unexpected("Loading was aborted"); \
                } \
                auto lineIndex = LINE_INDEX_TRANSFORM(i); \
                auto result = readInternal(fileLines[lineIndex], file); \
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

    if (not options.regex)
    {
        auto lineCheck = options.caseInsensitive
            ? &caseInsensitiveCheck
            : &caseSensitiveCheck;

        if (options.inverted)
        {
            if (parentBuffer.mType == cast(BufferType::filtered))
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
            if (parentBuffer.mType == cast(BufferType::filtered))
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
            if (parentBuffer.mType == cast(BufferType::filtered))
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
            if (parentBuffer.mType == cast(BufferType::filtered))
            {
                GREP_LOOP(RE2::PartialMatch(result.value(), re), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(RE2::PartialMatch(result.value(), re), FILE_LINE_INDEX_TRANSFORM);
            }
        }
    }

    return true;
}

void Buffer::Impl::filter(
    size_t start,
    size_t end,
    Buffer& parentBuffer)
{
    LineRefs lines;
    for (size_t i = start; i <= end; ++i)
    {
        auto lineIndex = i;

        if (parentBuffer.mType == cast(BufferType::filtered))
        {
            lineIndex = parentBuffer.mFilteredLines[lineIndex];
        }

        lines.push_back(lineIndex);
    }

    initialize(std::move(lines));
}

SearchResult Buffer::Impl::search(const SearchRequest& req, File& file)
{
    const auto lineCount = mLineCount;

    std::function<size_t(size_t)> lineIndexTransform;

    std::string_view pattern = req.pattern;

    if (mType == cast(BufferType::filtered))
    {
        lineIndexTransform = [this](size_t i){ return mFilteredLines[i]; };
    }
    else
    {
        lineIndexTransform = [](size_t i){ return i; };
    }

    auto& fileLines = *mFileLines;

    {
        auto result = readInternal(fileLines[lineIndexTransform(req.startLineIndex)], file);

        if (not result) [[unlikely]]
        {
            return {};
        }

        const auto& line = result.value();

        if (req.direction == SearchDirection::forward)
        {
            auto it = line.find(pattern, req.startLinePosition);

            if (it != line.npos and it != req.startLinePosition)
            {
                return SearchResult{
                    .valid = true,
                    .linePosition = unsigned(it),
                    .lineIndex = req.startLineIndex
                };
            }
        }
        else
        {
            auto it = line.rfind(pattern, req.startLinePosition);

            if (it != line.npos and it != req.startLinePosition)
            {
                return SearchResult{
                    .valid = true,
                    .linePosition = unsigned(it),
                    .lineIndex = req.startLineIndex
                };
            }
        }
    }

    if (req.direction == SearchDirection::forward)
    {
        for (size_t i = req.startLineIndex + 1; i < lineCount; ++i)
        {
            if (mStopFlag) [[unlikely]]
            {
                return SearchResult{.aborted = true};
            }

            auto result = readInternal(fileLines[lineIndexTransform(i)], file);

            if (not result) [[unlikely]]
            {
                return {};
            }

            const auto& line = result.value();

            auto it = line.find(pattern);

            if (it != line.npos)
            {
                return SearchResult{
                    .valid = true,
                    .linePosition = unsigned(it),
                    .lineIndex = i
                };
            }
        }
    }
    else
    {
        for (size_t i = req.startLineIndex - 1; i > 0; --i)
        {
            if (mStopFlag) [[unlikely]]
            {
                return SearchResult{.aborted = true};
            }

            auto result = readInternal(fileLines[lineIndexTransform(i)], file);

            if (not result) [[unlikely]]
            {
                return {};
            }

            const auto& line = result.value();
            auto it = line.find(pattern);

            if (it != line.npos)
            {
                return SearchResult{
                    .valid = true,
                    .linePosition = unsigned(it),
                    .lineIndex = i
                };
            }
        }
    }

    return {};
}

StringViewOrError Buffer::Impl::readInternal(Line line)
{
    return readInternal(line, mFile);
}

StringViewOrError Buffer::Impl::readInternal(Line line, File& file)
{
    if (line.len == 0)
    {
        return "";
    }

    if (not file.isAreaMapped(line.start, line.len)) [[unlikely]]
    {
        auto mappingLen = utils::min(BLOCK_SIZE, file.size() - line.start);

        if (auto result = file.remap(line.start, mappingLen); not result) [[unlikely]]
        {
            return std::unexpected(std::move(result.error()));
        }
    }

    return std::string_view(file.at(line.start), line.len);
}

}  // namespace core

#define LOG_HEADER "core::Buffer"
#include "buffer.hpp"

#include <algorithm>
#include <cstring>
#include <expected>
#include <string_view>
#include <type_traits>

#include "core/assert.hpp"
#include "core/config.hpp"
#include "core/context.hpp"
#include "core/line.hpp"
#include "core/logger.hpp"
#include "core/regex.hpp"
#include "core/thread.hpp"
#include "utils/format.hpp"
#include "utils/function_ref.hpp"
#include "utils/math.hpp"
#include "utils/memory.hpp"
#include "utils/time.hpp"
#include "utils/units.hpp"

namespace core
{

using Result = std::expected<bool, BufferError>;
using Results = std::vector<Result>;

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

struct Buffer::Impl final : Buffer
{
    Impl() = delete;

    constexpr static inline Impl& get(Buffer* b)
    {
        return *static_cast<Impl*>(b);
    }

    constexpr inline void setBusy()
    {
        mState = cast(State::busy);
    }

    constexpr inline void setIdle()
    {
        mState = cast(State::idle);
    }

    constexpr inline void setAborted()
    {
        mState = cast(State::aborted);
    }

    constexpr inline void setType(BufferType type)
    {
        mType = cast(type);
    }

    constexpr inline void stop()
    {
        if (mState == cast(State::busy)) [[unlikely]]
        {
            mStopFlag = true;
            while (mState == cast(State::busy));
            mStopFlag = false;
        }
    }

    void copyFromParent(Buffer& parentBuffer);
    void initialize(Lines&& lines);
    void initialize(LineRefs&& lineRefs);

    Result singleThreadedLoadFile();

    Result multiThreadedLoadFile(Context& context);

    Result readLines(
        File& file,
        size_t start,
        size_t end,
        Lines& lines);

    Result singleThreadedGrep(
        std::string pattern,
        GrepOptions options,
        Buffer& parentBuffer);

    Result multiThreadedGrep(
        std::string pattern,
        GrepOptions options,
        Buffer& parentBuffer,
        Context& context);

    Result grep(
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
    Impl::get(this).stop();
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

void Buffer::load(std::string path, Context& context, FinishedCallback callback)
{
    assert(mState == cast(State::uninitialized), utils::format("Buffer {} state is {}", this, stringify<State>(mState)));
    assert(mType == cast(BufferType::uninitialized), utils::format("Buffer {} type is {}", this, stringify<BufferType>(mType)));

    auto& impl = Impl::get(this);

    impl.setBusy();

    if (auto result = mFile.open(std::move(path)); not result) [[unlikely]]
    {
        impl.setAborted();
        callback(std::unexpected(BufferError::systemError(std::move(result.error()))));
        return;
    }

    async(
        [callback = std::move(callback), &impl, &context]
        {
            auto timer = utils::startTimeMeasurement();

            bool runMultiThreaded = impl.mFile.size() > context.config.bytesPerThread
                and context.config.maxThreads > 1;

            auto result = runMultiThreaded
                ? impl.multiThreadedLoadFile(context)
                : impl.singleThreadedLoadFile();

            if (result) [[likely]]
            {
                impl.setIdle();
                callback(timer.elapsed());
            }
            else
            {
                impl.setAborted();
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void Buffer::grep(std::string pattern, GrepOptions options, BufferId parentBufferId, Context& context, FinishedCallback callback)
{
    auto& impl = Impl::get(this);

    impl.setBusy();

    async(
        [pattern = std::move(pattern), callback = std::move(callback), options, parentBufferId, &context, &impl]
        {
            auto parentBuffer = getBuffer(parentBufferId, context);

            if (not parentBuffer) [[unlikely]]
            {
                impl.setAborted();
                callback(std::unexpected(BufferError::aborted("Parent buffer has been closed")));
                return;
            }

            impl.copyFromParent(*parentBuffer);

            auto timer = utils::startTimeMeasurement();

            bool runMultiThreaded = parentBuffer->mLineCount > context.config.linesPerThread
                and context.config.maxThreads > 1;

            auto result = runMultiThreaded
                ? impl.multiThreadedGrep(std::move(pattern), options, *parentBuffer, context)
                : impl.singleThreadedGrep(std::move(pattern), options, *parentBuffer);

            if (result) [[likely]]
            {
                impl.setIdle();
                callback(timer.elapsed());
            }
            else
            {
                impl.setAborted();
                callback(std::unexpected(std::move(result.error())));
            }
        });
}

void Buffer::filter(size_t start, size_t end, BufferId parentBufferId, Context& context, FinishedCallback callback)
{
    auto& impl = Impl::get(this);

    impl.setBusy();

    async(
        [start, end, callback = std::move(callback), parentBufferId, &context, &impl]
        {
            auto parentBuffer = getBuffer(parentBufferId, context);

            if (not parentBuffer) [[unlikely]]
            {
                impl.setAborted();
                callback(std::unexpected(BufferError::aborted("Parent buffer has been closed")));
                return;
            }

            impl.copyFromParent(*parentBuffer);

            auto timer = utils::startTimeMeasurement();

            impl.filter(start, end, *parentBuffer);

            impl.setIdle();
            callback(timer.elapsed());
        });
}

StringViewOrError Buffer::readLine(size_t i)
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

    return *result;
}

void Buffer::search(SearchRequest req, FinishedSearchCallback callback)
{
    auto& impl = Impl::get(this);

    impl.stop();
    impl.setBusy();

    async(
        [callback = std::move(callback), req = std::move(req), &impl]
        {
            auto file = impl.mFile;
            auto timer = utils::startTimeMeasurement();
            auto result = impl.search(req, file);
            impl.setIdle();
            callback(result, timer.elapsed());
        });
}

size_t Buffer::findClosestLine(size_t absoluteLineNumber)
{
    switch (mType)
    {
        case cast(BufferType::uninitialized):
        case cast(BufferType::base):
            return absoluteLineNumber;
        case cast(BufferType::filtered):
            for (size_t i = 0; i < mFilteredLines.size(); ++i)
            {
                if (absoluteLineNumber <= mFilteredLines[i])
                {
                    return i;
                }
            }
            return mFilteredLines.size() - 1;
    }
    return 0;
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

const std::string& Buffer::filePath() const
{
    assert(mType != cast(BufferType::uninitialized), utils::format("Buffer {} type is uninitialized", this));
    return mFile.path();
}

void Buffer::Impl::copyFromParent(Buffer& parentBuffer)
{
    mFile = parentBuffer.mFile;
    mFileLines = parentBuffer.mFileLines;
}

void Buffer::Impl::initialize(Lines&& lines)
{
    mLineCount = lines.size();
    utils::constructAt(&mOwnLines, std::move(lines));
    mFileLines = &mOwnLines;
    setType(BufferType::base);
}

void Buffer::Impl::initialize(LineRefs&& lines)
{
    mLineCount = lines.size();
    utils::constructAt(&mFilteredLines, std::move(lines));
    setType(BufferType::filtered);
}

Result Buffer::Impl::singleThreadedLoadFile()
{
    Lines lines;

    auto result = readLines(mFile, 0, mFile.size(), lines);

    if (not result)
    {
        return result;
    }

    const auto nextLineStart = lines.empty()
        ? 0
        : lines.back().start + lines.back().len + 1;

    if (nextLineStart < mFile.size()) [[unlikely]]
    {
        lines.emplace_back(Line{.start = nextLineStart, .len = mFile.size() - nextLineStart});
    }

    initialize(std::move(lines));

    return true;
}

Result Buffer::Impl::multiThreadedLoadFile(Context& context)
{
    const auto fileSize = mFile.size();
    const auto maxThreads = context.config.maxThreads.get();
    const auto bytesPerThread = context.config.bytesPerThread.get();

    const auto threadCount = utils::min(
        (fileSize + bytesPerThread - 1) / bytesPerThread,
        size_t(maxThreads));

    logger.info() << "using " << threadCount << " threads";

    Tasks tasks(threadCount);
    Results results(threadCount);
    std::vector<Lines> linesPerThread(threadCount);

    for (size_t i = 0; i < threadCount; ++i)
    {
        const auto start = (fileSize / threadCount) * i;
        const auto end = i == threadCount - 1
            ? fileSize
            : (fileSize / threadCount) * (i + 1);

        auto& threadLines = linesPerThread[i];
        auto& threadResult = results[i];

        tasks[i] =
            [start, end, &threadLines, &threadResult, threadFile = mFile, this] mutable
            {
                threadResult = readLines(
                    threadFile,
                    start,
                    end,
                    threadLines);
            };
    }

    executeInParallelAndWait(std::move(tasks));

    Result result(true);
    size_t lineCount = threadCount;

    for (size_t i = 0; i < threadCount; ++i)
    {
        if (not results[i]) [[unlikely]]
        {
            result = std::move(results[i]);
        }
        lineCount += linesPerThread[i].size();
    }

    if (not result)
    {
        return result;
    }

    Lines lines(std::move(linesPerThread[0]));
    lines.reserve(lineCount);

    for (size_t i = 1; i < threadCount; ++i)
    {
        if (i < threadCount - 1 and not lines.empty())
        {
            auto& prevLine = lines.back();
            auto& nextLine = linesPerThread[i].front();

            if (prevLine.start + prevLine.len + 1 < nextLine.start)
            {
                const auto diff = nextLine.start - (prevLine.start + prevLine.len + 1);
                nextLine.start -= diff;
                nextLine.len += diff;
            }
        }

        std::copy(
            linesPerThread[i].begin(), linesPerThread[i].end(),
            std::back_inserter(lines));

        linesPerThread[i] = Lines{};
    }

    const auto nextLineStart = lines.empty()
        ? 0
        : lines.back().start + lines.back().len + 1;

    if (nextLineStart < mFile.size()) [[unlikely]]
    {
        lines.emplace_back(Line{.start = nextLineStart, .len = mFile.size() - nextLineStart});
    }

    initialize(std::move(lines));

    return true;
}

Result Buffer::Impl::readLines(
    File& file,
    size_t start,
    size_t end,
    Lines& lines)
{
    auto sizeLeft{end - start};
    auto offset{start};
    auto lineStart{start};

    while (sizeLeft)
    {
        auto toRead = utils::min(sizeLeft, BLOCK_SIZE);

        if (auto result = file.remap(offset, toRead); not result) [[unlikely]]
        {
            return std::unexpected(BufferError::systemError(std::move(result.error())));
        }

        const auto text = file.at(offset);

        for (size_t i = 0; i < toRead; ++i)
        {
            if (text[i] == '\n')
            {
                if (mStopFlag) [[unlikely]]
                {
                    return std::unexpected(BufferError::aborted("Loading was aborted"));
                }

                lines.emplace_back(Line{.start = lineStart, .len = offset + i - lineStart});
                lineStart = offset + i + 1;
            }
        }

        sizeLeft -= toRead;
        offset += toRead;
    }

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

Result Buffer::Impl::singleThreadedGrep(
    std::string pattern,
    GrepOptions options,
    Buffer& parentBuffer)
{
    LineRefs lines;

    auto result = grep(
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

Result Buffer::Impl::multiThreadedGrep(
    std::string pattern,
    GrepOptions options,
    Buffer& parentBuffer,
    Context& context)
{
    const auto lineCount = parentBuffer.mLineCount;
    const uint8_t maxThreads = context.config.maxThreads;
    const size_t linesPerThread = context.config.linesPerThread;

    const auto threadCount = utils::min(
        (lineCount + linesPerThread - 1) / linesPerThread,
        size_t(maxThreads));

    logger.info() << "using " << threadCount << " threads";

    Tasks tasks(threadCount);
    Results results(threadCount);
    std::vector<LineRefs> lineRefsPerThread(threadCount);

    for (size_t i = 0; i < threadCount; ++i)
    {
        const auto start = (lineCount / threadCount) * i;
        const auto end = i == threadCount - 1
            ? lineCount
            : (lineCount / threadCount) * (i + 1);

        auto& threadLines = lineRefsPerThread[i];
        auto& threadResult = results[i];

        tasks[i] =
            [pattern, options, &parentBuffer, start, end,
                &threadLines, &threadResult,
                threadFile = mFile,
                this] mutable
            {
                threadResult = grep(
                    std::move(pattern),
                    options,
                    parentBuffer,
                    threadFile,
                    start,
                    end,
                    threadLines);
            };
    }

    executeInParallelAndWait(std::move(tasks));

    Result result(true);
    size_t summedLineCount = 0;

    for (size_t i = 0; i < threadCount; ++i)
    {
        if (not results[i]) [[unlikely]]
        {
            result = std::move(results[i]);
        }
        summedLineCount += lineRefsPerThread[i].size();
    }

    if (not result)
    {
        return result;
    }

    LineRefs lines(std::move(lineRefsPerThread[0]));
    lines.reserve(summedLineCount);

    for (size_t i = 1; i < threadCount; ++i)
    {
        std::copy(
            lineRefsPerThread[i].begin(), lineRefsPerThread[i].end(),
            std::back_inserter(lines));

        lineRefsPerThread[i] = LineRefs{};
    }

    initialize(std::move(lines));

    return true;
}

Result Buffer::Impl::grep(
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
                    return std::unexpected(BufferError::aborted("Loading was aborted")); \
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
                GREP_LOOP(not lineCheck(*result, pattern), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(not lineCheck(*result, pattern), FILE_LINE_INDEX_TRANSFORM);
            }
        }
        else
        {
            if (parentBuffer.mType == cast(BufferType::filtered))
            {
                GREP_LOOP(lineCheck(*result, pattern), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(lineCheck(*result, pattern), FILE_LINE_INDEX_TRANSFORM);
            }
        }
    }
    else
    {
        Regex re(std::move(pattern), options.caseInsensitive);

        if (not re.ok()) [[unlikely]]
        {
            return std::unexpected(BufferError::regexError(re.error()));
        }

        if (options.inverted)
        {
            if (parentBuffer.mType == cast(BufferType::filtered))
            {
                GREP_LOOP(not re.partialMatch(*result), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(not re.partialMatch(*result), FILE_LINE_INDEX_TRANSFORM);
            }
        }
        else
        {
            if (parentBuffer.mType == cast(BufferType::filtered))
            {
                GREP_LOOP(re.partialMatch(*result), FILTERED_LINE_INDEX_TRANSFORM);
            }
            else
            {
                GREP_LOOP(re.partialMatch(*result), FILE_LINE_INDEX_TRANSFORM);
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

    utils::FunctionRef<size_t(size_t)> lineIndexTransform;

    std::string_view pattern = req.pattern;

    auto filteredLinesTransform = [this](size_t i){ return mFilteredLines[i]; };
    auto fileLinesTransform = [](size_t i){ return i; };

    if (mType == cast(BufferType::filtered))
    {
        lineIndexTransform = filteredLinesTransform;
    }
    else
    {
        lineIndexTransform = fileLinesTransform;
    }

    auto& fileLines = *mFileLines;

    {
        auto result = readInternal(fileLines[lineIndexTransform(req.startLineIndex)], file);

        if (not result) [[unlikely]]
        {
            return {};
        }

        const auto& line = *result;

        if (req.direction == SearchDirection::forward)
        {
            auto startLinePosition = req.startLinePosition + static_cast<int>(req.continuation);
            auto it = line.find(pattern, startLinePosition);

            if (it != line.npos)
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
            if (req.startLinePosition > 1)
            {
                auto startLinePosition = req.startLinePosition - static_cast<int>(req.continuation);
                auto it = line.rfind(pattern, startLinePosition);

                if (it != line.npos)
                {
                    return SearchResult{
                        .valid = true,
                        .linePosition = unsigned(it),
                        .lineIndex = req.startLineIndex
                    };
                }
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

            const auto& line = *result;

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
        for (size_t i = req.startLineIndex - 1; static_cast<long>(i) >= 0; --i)
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

            const auto& line = *result;
            auto it = line.rfind(pattern, result->size());

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
            return std::unexpected(BufferError::systemError(std::move(result.error())));
        }
    }

    return std::string_view(file.at(line.start), line.len);
}

BufferError::BufferError(Type error, std::string message)
    : mMessage(message)
{
    mMessage[mMessage.size()] = char(error);
}

BufferError::operator Type() const
{
    return static_cast<Type>(mMessage[mMessage.size()]);
}

bool BufferError::operator==(Type error) const
{
    return static_cast<Type>(mMessage[mMessage.size()]) == error;
}

std::string_view BufferError::message() const
{
    return std::string_view(mMessage.begin(), mMessage.end());
}

BufferError BufferError::aborted(std::string message)
{
    return BufferError(Type::Aborted, std::move(message));
}

BufferError BufferError::systemError(std::string message)
{
    return BufferError(Type::SystemError, std::move(message));
}

BufferError BufferError::regexError(std::string message)
{
    return BufferError(Type::RegexError, std::move(message));
}

utils::Buffer& operator<<(utils::Buffer& buf, const BufferError& error)
{
    switch (error)
    {
        case BufferError::Aborted:
            buf << "[Aborted] ";
            break;

        case BufferError::SystemError:
            buf << "[System error] ";
            break;

        case BufferError::RegexError:
            buf << "[Regex error] ";
            break;

        default:
            buf << "[Unknown error] ";
    }
    return buf << error.message();
}

}  // namespace core

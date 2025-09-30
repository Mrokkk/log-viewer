#pragma once

#include <atomic>
#include <cstdlib>
#include <expected>
#include <functional>

#include "core/buffers.hpp"
#include "core/file.hpp"
#include "core/fwd.hpp"
#include "core/grep_options.hpp"
#include "core/line.hpp"
#include "utils/fwd.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct BufferError final
{
    enum Type : char
    {
        Aborted = 1,
        SystemError,
        RegexError,
    };

    operator Type() const;

    bool operator==(Type error) const;

    std::string_view message() const;

    static BufferError aborted(std::string message);
    static BufferError systemError(std::string message);
    static BufferError regexError(std::string message);

private:
    BufferError(Type error, std::string message);

    std::string mMessage;
};

utils::Buffer& operator<<(utils::Buffer& buf, const BufferError& error);

enum class SearchDirection
{
    forward,
    backward,
};

struct SearchResult
{
    const bool     valid = false;
    const bool     aborted = false;
    const unsigned linePosition;
    const size_t   lineIndex;
};

struct SearchRequest
{
    const SearchDirection direction;
    const bool            continuation;
    const size_t          startLineIndex;
    const size_t          startLinePosition;
    std::string           pattern;
};

using TimeOrError = std::expected<float, BufferError>;
using StringViewOrError = std::expected<std::string_view, BufferError>;
using FinishedCallback = std::function<void(TimeOrError)>;
using FinishedSearchCallback = std::function<void(SearchResult, float)>;

struct Buffer : utils::Immobile
{
    Buffer();
    ~Buffer();

    void load(std::string path, Context& context, FinishedCallback callback);
    void grep(std::string pattern, GrepOptions options, BufferId parentBufferId, Context& context, FinishedCallback callback);
    void filter(size_t start, size_t end, BufferId parentBufferId, Context& context, FinishedCallback callback);
    StringViewOrError readLine(size_t i);

    void search(SearchRequest req, FinishedSearchCallback callback);

    size_t findClosestLine(size_t absoluteLineNumber);

    size_t absoluteLineNumber(size_t lineIndex) const;

    const std::string& filePath() const;

    constexpr size_t fileLineCount() const
    {
        return mFileLines->size();
    }

    constexpr size_t lineCount() const
    {
        return mLineCount;
    }

private:
    struct Impl;

    std::atomic_bool mStopFlag;
    std::atomic_char mState;
    std::atomic_char mType;
    File             mFile;
    size_t           mLineCount;
    Lines*           mFileLines;
    union
    {
        Lines        mOwnLines;
        LineRefs     mFilteredLines;
    };
};

}  // namespace core

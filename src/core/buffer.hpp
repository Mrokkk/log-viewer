#pragma once

#include <atomic>
#include <cstdlib>
#include <expected>
#include <functional>

#include "core/buffers.hpp"
#include "core/error.hpp"
#include "core/file.hpp"
#include "core/fwd.hpp"
#include "core/grep_options.hpp"
#include "core/line.hpp"
#include "utils/immobile.hpp"

namespace core
{

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
    const size_t          startLineIndex;
    const size_t          startLinePosition;
    std::string           pattern;
};

using TimeOrError = std::expected<float, Error>;
using StringOrError = std::expected<std::string, Error>;
using FinishedCallback = std::function<void(TimeOrError)>;
using FinishedSearchCallback = std::function<void(SearchResult, float)>;

struct Buffer : utils::Immobile
{
    Buffer();
    ~Buffer();

    void load(std::string path, Context& context, FinishedCallback callback);
    void grep(std::string pattern, GrepOptions options, BufferId parentBufferId, Context& context, FinishedCallback callback);
    void filter(size_t start, size_t end, BufferId parentBufferId, Context& context, FinishedCallback callback);
    StringOrError readLine(size_t i);

    void search(SearchRequest req, FinishedSearchCallback callback);

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

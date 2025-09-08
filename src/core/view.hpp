#pragma once

#include <atomic>
#include <cstdlib>
#include <expected>
#include <functional>

#include "core/file.hpp"
#include "core/fwd.hpp"
#include "core/grep_options.hpp"
#include "core/line.hpp"
#include "core/views.hpp"
#include "utils/immobile.hpp"

namespace core
{

using TimeOrError = std::expected<float, std::string>;
using StringOrError = std::expected<std::string, std::string>;
using ViewLoadedCallback = std::function<void(TimeOrError)>;

struct View : utils::Immobile
{
    View();
    ~View();

    void load(std::string path, Context& context, ViewLoadedCallback callback);
    void grep(std::string pattern, GrepOptions options, ViewId parentViewId, Context& context, ViewLoadedCallback callback);
    void filter(size_t start, size_t end, ViewId parentViewId, Context& context, ViewLoadedCallback callback);
    StringOrError readLine(size_t i);

    size_t absoluteLineNumber(size_t lineIndex) const;

    size_t fileLineCount() const;
    const std::string& filePath() const;

    size_t lineCount() const
    {
        return mLineCount;
    }

private:
    void loading();
    void copyFromParent(View& parentView);
    void initialize(Lines&& lines);
    void initialize(LineRefs&& lineRefs);

    std::expected<bool, std::string> loadFile();

    std::expected<bool, std::string> grep(
        std::string pattern,
        GrepOptions options,
        View& parentView);

    void filter(
        size_t start,
        size_t end,
        View& parentView);

    std::expected<std::string_view, std::string> readInternal(Line line);

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

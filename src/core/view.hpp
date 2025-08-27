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
        return lineCount_;
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

    std::atomic_bool stopFlag_;
    std::atomic_char state_;
    std::atomic_char type_;
    File             file_;
    size_t           lineCount_;
    Lines*           fileLines_;
    union
    {
        Lines        ownLines_;
        LineRefs     greppedLines_;
    };
};

}  // namespace core

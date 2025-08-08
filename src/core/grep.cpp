#include "grep.hpp"

#include <functional>
#include <thread>

#include <re2/re2.h>

#include "utils/time.hpp"

namespace core
{

static bool caseSensitiveCheck(const std::string& line, const std::string& pattern)
{
    return line.contains(pattern);
}

static bool caseInsensitiveCheck(const std::string& line, const std::string& pattern)
{
    return std::search(
        line.begin(), line.end(),
        pattern.begin(), pattern.end(),
        [](char c1, char c2)
        {
            return std::tolower(c1) == std::tolower(c2);
        }) != line.end();
}

static LineRefs grep(
    const std::string& pattern,
    const GrepOptions& options,
    const LineRefs& filter,
    MappedFile& file)
{
    LineRefs lines;
    size_t lineCount;
    std::function<size_t(size_t)> lineNumberTransform;

    if (not filter.empty())
    {
        lineNumberTransform = [&filter](size_t i){ return filter[i]; };
        lineCount = filter.size();
    }
    else
    {
        lineNumberTransform = [](size_t i){ return i; };
        lineCount = file.lineCount();
    }

    if (not options.regex)
    {
        auto lineCheck = options.caseInsensitive
            ? &caseInsensitiveCheck
            : &caseSensitiveCheck;

        if (options.inverted)
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (not lineCheck(file[lineIndex], pattern))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (lineCheck(file[lineIndex], pattern))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
    }
    else
    {
        RE2::Options reOptions;
        reOptions.set_log_errors(false);

        if (options.caseInsensitive)
        {
            reOptions.set_case_sensitive(false);
        }

        RE2 re(std::move(pattern), reOptions);

        if (options.inverted)
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (not RE2::PartialMatch(file[lineIndex], re))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (RE2::PartialMatch(file[lineIndex], re))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
    }

    return lines;
}

void asyncGrep(
    std::string pattern,
    GrepOptions options,
    const LineRefs& filter,
    MappedFile& file,
    std::function<void(LineRefs, float)> callback)
{
    std::thread(
        [pattern = std::move(pattern), &filter, &file, callback = std::move(callback), options]
        {
            LineRefs lines;
            auto time = utils::measureTime(
                [&lines, &pattern, &filter, &file, options]
                {
                    lines = grep(pattern, options, filter, file);
                });
            callback(std::move(lines), time);
        }).detach();
}

}  // namespace core

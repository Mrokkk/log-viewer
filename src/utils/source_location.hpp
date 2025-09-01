#pragma once

#include <cstddef>
#include <string_view>

namespace utils
{

struct SourceLocation
{
    static consteval SourceLocation current(
        const char* file = __builtin_FILE(),
        const char* func = __builtin_FUNCTION(),
        size_t line = __builtin_LINE())
    {
        std::string_view sv(file);
        sv.remove_prefix(sv.find("src"));

        return SourceLocation{
            .file = sv.data(),
            .func = func,
            .line = line,
        };
    }

    const char*  file;
    const char*  func;
    size_t       line;
};

}  // namespace utils

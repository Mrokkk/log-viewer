#pragma once

#include <ctime>

#include "utils/buffer.hpp"

namespace utils
{

struct FormatTime
{
    struct tm* tm;
    const char* fmt;
};

constexpr inline FormatTime formatTime(struct tm* tm, const char* fmt)
{
    return {tm, fmt};
}

utils::Buffer& operator<<(utils::Buffer& buf, const FormatTime& formatTime);

}  // namespace utils

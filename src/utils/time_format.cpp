#include "time_format.hpp"

#include <ctime>

namespace utils
{

utils::Buffer& operator<<(utils::Buffer& buf, const FormatTime& formatTime)
{
    char buffer[64];
    auto len = strftime(buffer, sizeof(buffer), formatTime.fmt, formatTime.tm);
    return buf << std::string_view(buffer, len);
}

}  // namespace utils

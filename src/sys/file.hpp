#pragma once

#include <string>

namespace sys
{

struct File final
{
    const std::string path;
    const size_t      size;
    const int         fd;
};

}  // namespace sys

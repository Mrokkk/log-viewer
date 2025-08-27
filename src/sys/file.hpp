#pragma once

#include <expected>
#include <string>

#include "sys/common.hpp"

namespace sys
{

struct File final
{
    std::string    path;
    size_t         size;
    FileDescriptor fd;
};

using MaybeFile = std::expected<File, Error>;

}  // namespace sys

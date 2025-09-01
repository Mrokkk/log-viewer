#pragma once

#include "utils/fwd.hpp"

namespace core
{

enum class Type
{
    any,
    variadic,
    null,
    integer,
    string,
    boolean,
};

utils::Buffer& operator<<(utils::Buffer& buf, const Type type);

}  // namespace core

#pragma once

#include <iosfwd>

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

std::ostream& operator<<(std::ostream& os, const Type type);

}  // namespace core

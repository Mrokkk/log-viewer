#include "type.hpp"

#include <ostream>

namespace core
{

std::ostream& operator<<(std::ostream& os, const Type type)
{
#define TYPE_PRINT(type) \
    case Type::type: return os << #type
    switch (type)
    {
        TYPE_PRINT(any);
        TYPE_PRINT(variadic);
        TYPE_PRINT(null);
        TYPE_PRINT(integer);
        TYPE_PRINT(string);
        TYPE_PRINT(boolean);
        default:
            return os << "unknown{" << static_cast<int>(type) << '}';
    }
}

}  // namespace core

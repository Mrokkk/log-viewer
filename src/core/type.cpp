#include "type.hpp"

#include "utils/buffer.hpp"

namespace core
{

utils::Buffer& operator<<(utils::Buffer& buf, const Type type)
{
#define TYPE_PRINT(type) \
    case Type::type: return buf << #type
    switch (type)
    {
        TYPE_PRINT(any);
        TYPE_PRINT(variadic);
        TYPE_PRINT(null);
        TYPE_PRINT(integer);
        TYPE_PRINT(string);
        TYPE_PRINT(boolean);
        default:
            return buf << "unknown{" << static_cast<int>(type) << '}';
    }
}

}  // namespace core

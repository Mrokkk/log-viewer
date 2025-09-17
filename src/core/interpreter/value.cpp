#include "value.hpp"

#include "core/interpreter/object.hpp"
#include "utils/buffer.hpp"

namespace core::interpreter
{

OpResult Value::assign(const Value& other)
{
    const auto thisType = type();
    const auto otherType = other.type();

    if (thisType != otherType) [[unlikely]]
    {
        utils::Buffer buf;
        buf << "Cannot assign value of " << other.type() << " to " << thisType;

        return OpResult::error(buf.view());
    }

    *this = other;
    return OpResult::success;
}

utils::Buffer& operator<<(utils::Buffer& buf, Value::Type type)
{
#define PRINT(TYPE) \
    case Value::Type::TYPE: return buf << #TYPE
    switch (type)
    {
        PRINT(null);
        PRINT(integer);
        PRINT(boolean);
        PRINT(object);
    }
    return buf << "unknown{" << static_cast<int>(type) << '}';
}

static void valueToStream(utils::Buffer& buf, const Value& value)
{
    switch (value.type())
    {
        case Value::Type::null:
            buf << "null";
            break;

        case Value::Type::boolean:
            buf << *value.boolean();
            break;

        case Value::Type::integer:
            buf << *value.integer();
            break;

        case Value::Type::object:
            buf << *value.object();
            break;

        default:
            break;
    }
}

std::string getValueString(const Value& value)
{
    utils::Buffer buf;
    valueToStream(buf, value);
    return buf.str();
}

utils::Buffer& operator<<(utils::Buffer& buf, const Value& object)
{
    valueToStream(buf, object);
    return buf;
}

}  // namespace core::interpreter

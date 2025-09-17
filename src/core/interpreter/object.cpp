#include "object.hpp"

#include <string>

#include "utils/buffer.hpp"
#include "utils/memory.hpp"

namespace core::interpreter
{

Object Object::create(std::string_view sv)
{
    auto obj = static_cast<ObjectData*>(std::malloc(sizeof(ObjectData) + sv.size()));

    utils::constructAt(obj, Type::string);

    obj->mString.len = sv.size();

    std::memcpy(obj->mString.string, sv.data(), sv.size());

    return Object(obj);
}

Object Object::create(Command& command)
{
    auto obj = static_cast<ObjectData*>(std::malloc(sizeof(ObjectData)));

    utils::constructAt(obj, Type::command);

    obj->mCommand = &command;

    return Object(obj);
}

Object::~Object()
{
    if (mData and --mData->mRefCount == 0)
    {
        std::free(mData);
    }
}

std::string_view Object::stringView() const
{
    return std::string_view(mData->mString.string, mData->mString.len);
}

std::string Object::string() const
{
    return std::string(mData->mString.string, mData->mString.len);
}

utils::Buffer& operator<<(utils::Buffer& buf, const Object& obj)
{
    switch (obj.type())
    {
        case Object::Type::string:
            return buf << obj.stringView();
        case Object::Type::command:
            return buf << "<internal command>";
        default:
            return buf << "<invalid>";
    }
}

}  // namespace core::interpreter

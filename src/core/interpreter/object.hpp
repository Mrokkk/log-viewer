#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include "core/interpreter/fwd.hpp"
#include "utils/buffer.hpp"
#include "utils/immobile.hpp"

namespace core::interpreter
{

struct Object final
{
    enum class Type : uint8_t
    {
        string,
        command,
    };

private:
    struct String
    {
        size_t len;
        char   string[];
    };

    using RefCount = uint32_t;

    struct ObjectData final : utils::Immobile
    {
        constexpr ObjectData(Type type)
            : mType(type)
            , mRefCount(1)
        {
        }

        Type     mType;
        RefCount mRefCount;

        union
        {
            String   mString;
            Command* mCommand;
        };
    };

public:
    static Object create(std::string_view sv);
    static Object create(Command& command);

    constexpr Object()
        : mData(nullptr)
    {
    }

    constexpr Object(ObjectData* data)
        : mData(data)
    {
    }

    constexpr Object(const Object& other)
        : mData(other.mData)
    {
        ++mData->mRefCount;
    }

    constexpr Object(Object&& other)
        : mData(other.mData)
    {
        other.mData = nullptr;
    }

    ~Object();

    constexpr operator bool() const { return mData != nullptr; }

    constexpr auto type() const { return mData->mType; }

    std::string_view stringView() const;
    std::string      string()     const;

private:
    ObjectData* mData;
};

utils::Buffer& operator<<(utils::Buffer& buf, const Object& obj);

}  // namespace core::interpreter

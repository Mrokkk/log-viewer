#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "core/interpreter/object.hpp"
#include "utils/fwd.hpp"
#include "utils/maybe.hpp"
#include "utils/memory.hpp"

namespace core::interpreter
{

struct [[nodiscard]] OpResult
{
    constexpr static inline struct Success{} success = {};

    constexpr OpResult(Success)
    {
    }

    template <typename T>
    requires std::convertible_to<T, std::string_view>
    constexpr OpResult(std::unexpected<T> error)
        : mError(Object::create(error.error()))
    {
    }

    constexpr OpResult(OpResult&& other)
        : mError(std::move(other.mError))
    {
    }

    constexpr operator bool() const { return not mError; }
    constexpr bool operator==(Success) const { return not mError; }
    constexpr auto error() const { return mError.stringView(); }

    constexpr static OpResult error(const std::string_view& sv)
    {
        return OpResult(std::unexpected(sv));
    }

private:
    Object mError;
};

struct Value
{
    enum class Type : uint8_t
    {
        null,
        integer,
        boolean,
        object,
    };

    constexpr Value()
        : mType(Type::null)
        , mPtr(nullptr)
    {
    }

    constexpr Value(std::nullptr_t)
        : mType(Type::null)
        , mPtr(nullptr)
    {
    }

    constexpr Value(long i)
        : mType(Type::integer)
        , mInteger(i)
    {
    }

    constexpr Value(bool b)
        : mType(Type::boolean)
        , mBoolean(b)
    {
    }

    constexpr Value(Object o)
        : mType(Type::object)
        , mObject(o)
    {
    }

    constexpr Value(const Value& other)
        : mType(other.mType)
    {
        if (mType == Type::object)
        {
            utils::constructAt(&mObject, other.mObject);
        }
        else
        {
            mPtr = other.mPtr;
        }
    }

    constexpr Value(Value&& other)
        : mType(other.mType)
    {
        if (mType == Type::object)
        {
            utils::constructAt(&mObject, std::move(other.mObject));
        }
        else
        {
            mPtr = other.mPtr;
        }
        other.clear();
    }

    constexpr Value& operator=(const Value& other)
    {
        reset();

        mType = other.mType;
        mPtr = other.mPtr;

        if (mType == Type::object)
        {
            utils::constructAt(&mObject, other.mObject);
        }
        else
        {
            mPtr = other.mPtr;
        }

        return *this;
    }

    constexpr Value& operator=(Value&& other)
    {
        reset();

        mType = other.mType;

        if (mType == Type::object)
        {
            utils::constructAt(&mObject, std::move(other.mObject));
        }
        else
        {
            mPtr = other.mPtr;
        }

        other.clear();

        return *this;
    }

    constexpr ~Value()
    {
        reset();
    }

    constexpr auto type() const { return mType; }

    constexpr utils::Maybe<long> integer() const
    {
        switch (type())
        {
            case Type::integer:
                return mInteger;
            [[unlikely]] default:
                return {};
        }
    }

    constexpr utils::Maybe<bool> boolean() const
    {
        switch (type())
        {
            case Type::boolean:
                return mBoolean;
            [[unlikely]] default:
                return {};
        }
    }

    constexpr utils::Maybe<Object> object() const
    {
        switch (type())
        {
            case Type::object:
                return mObject;
            [[unlikely]] default:
                return {};
        }
    }

    constexpr utils::Maybe<std::string_view> stringView() const
    {
        switch (type())
        {
            case Type::object:
                return mObject.stringView();
            [[unlikely]] default:
                return {};
        }
    }

    constexpr utils::Maybe<std::string> string() const
    {
        switch (type())
        {
            case Type::object:
                return mObject.string();
            [[unlikely]] default:
                return {};
        }
    }

    constexpr void reset()
    {
        if (mType == Type::object)
        {
            utils::destroyAt(&mObject);
        }
        clear();
    }

    OpResult assign(const Value& other);
    OpResult call();

private:
    constexpr void clear()
    {
        mType = Type::null;
        mPtr = nullptr;
    }

    Type mType;

    union
    {
        void*  mPtr;
        long   mInteger;
        bool   mBoolean;
        Object mObject;
    };

    static_assert(sizeof(Object) == sizeof(void*), "Size of Object should be equal to size of pointer");
};

using Values = std::vector<Value>;

utils::Buffer& operator<<(utils::Buffer& buf, Value::Type type);
utils::Buffer& operator<<(utils::Buffer& buf, const Value& object);
std::string getValueString(const Value& value);

}  // namespace core::interpreter

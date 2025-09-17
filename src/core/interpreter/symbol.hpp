#pragma once

#include <string>

#include "core/fwd.hpp"
#include "core/interpreter/value.hpp"
#include "utils/buffer.hpp"
#include "utils/fwd.hpp"

namespace core::interpreter
{

struct Symbol
{
    enum class Access : uint8_t
    {
        readOnly,
        readWrite,
    };

    constexpr Symbol(Access a, Value v)
        : mAccess(a)
        , mValue(v)
        , mHelp(nullptr)
    {
    }

    constexpr Symbol(const Symbol& other)
        : mAccess(other.mAccess)
        , mValue(other.mValue)
        , mHelp(nullptr)
    {
    }

    constexpr Symbol(Symbol&& other)
        : mAccess(other.mAccess)
        , mValue(std::move(other.mValue))
        , mHelp(other.mHelp)
    {
    }

    constexpr Symbol& operator=(const Symbol& other)
    {
        mAccess = other.mAccess;
        mValue = other.mValue;
        mHelp = other.mHelp;
        return *this;
    }

    constexpr virtual ~Symbol() = default;

    virtual OpResult assign(Value value, Context& context);

    constexpr inline auto& value() const { return mValue; }
    constexpr inline auto& value()       { return mValue; }
    constexpr inline auto help()   const { return mHelp; }

    constexpr Symbol& setHelp(const char* help)
    {
        mHelp = help;
        return *this;
    }

protected:
    Access      mAccess;
    Value       mValue;
    const char* mHelp;
};

struct Symbols final
{
    Symbols() = delete;

    static Symbol* find(const std::string& name);

    static void addReadWrite(
        std::string name,
        Value value);

    static void addReadOnly(
        std::string name,
        Value value);

    static void add(std::string name, Symbol& symbol);
};

utils::Buffer& operator<<(utils::Buffer& buf, const Symbol& symbol);

}  // namespace core::interpreter

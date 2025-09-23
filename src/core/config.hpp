#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>

#include "core/fwd.hpp"
#include "core/interpreter/symbol.hpp"
#include "utils/bitflag.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Config;

DEFINE_BITFLAG(ConfigFlags, uint8_t,
{
    reloadAllWindows,
});

namespace detail
{

template <typename T>
concept Integral = std::integral<T> and not std::is_same_v<T, bool>;

template <typename T>
struct Limits
{
};

template <typename T>
requires Integral<T>
struct Limits<T>
{
    const T min;
    const T max;
};

template <typename T>
struct ConfigVariable final : interpreter::Symbol
{
    constexpr inline operator T() const { return get(); }
    constexpr inline operator T()       { return get(); }

    constexpr inline auto get() const
    {
        if constexpr (std::is_same_v<T, std::string_view>)
        {
            return value().object()->stringView();
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return *value().boolean();
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return static_cast<T>(*value().integer());
        }
        else
        {
            static_assert(false, "Invalid type");
        }
    }

    ConfigVariable& setFlag(ConfigFlags flags);

    interpreter::OpResult assign(interpreter::Value newValue, Context& context) override;

private:
    template <typename U>
    requires std::convertible_to<U, std::string_view>
    constexpr static interpreter::Value createValue(U value)
    {
        return interpreter::Value(interpreter::Object::create(std::string_view(value)));
    }

    constexpr static interpreter::Value createValue(T value)
        requires std::is_same_v<T, bool>
    {
        return interpreter::Value(value);
    }

    constexpr static interpreter::Value createValue(T value)
        requires detail::Integral<T>
    {
        return interpreter::Value(long(value));
    }

    void onChange(Context& context);

private:
    friend Config;

    template <typename U>
    requires (not detail::Integral<U>)
    constexpr ConfigVariable(U defaultValue)
        : Symbol(Symbol::Access::readWrite, createValue(defaultValue))
    {
    }

    template <typename U>
    requires detail::Integral<U>
    constexpr ConfigVariable(
        U defaultValue,
        T min = std::numeric_limits<T>::min(),
        T max = std::numeric_limits<T>::max())
        : Symbol(Symbol::Access::readWrite, createValue(defaultValue))
        , mLimits{min, max}
    {
    }

    ConfigFlags       mFlags;
    detail::Limits<T> mLimits;
};

}  // namespace detail

struct Config : utils::Immobile
{
    using Uint8  = detail::ConfigVariable<uint8_t>;
    using Uint32 = detail::ConfigVariable<uint32_t>;
    using Size   = detail::ConfigVariable<size_t>;
    using Bool   = detail::ConfigVariable<bool>;
    using String = detail::ConfigVariable<std::string_view>;

    Size   maxThreads;
    Size   linesPerThread;
    Size   bytesPerThread;
    Bool   showLineNumbers;
    Bool   absoluteLineNumbers;
    Bool   highlightSearch;
    Uint8  scrollJump;
    Uint8  scrollOff;
    Uint8  fastMoveLen;
    Uint8  tabWidth;
    Uint32 highlightColor;
    String lineNumberSeparator;
    String tabChar;

private:
    friend Context;
    Config();
};

}  // namespace core

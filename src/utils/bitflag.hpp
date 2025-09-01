#pragma once

#include "utils/enum_traits.hpp"

namespace utils
{

namespace detail
{

template <typename T, typename U>
requires IsEnum<U>
constexpr T bitFlagValue(U v)
{
    return 1 << static_cast<int>(v);
}

}  // namespace detail

template <typename T>
struct BitFlag
{
    using Underlying = T;

    constexpr BitFlag() : value(0) {}

    constexpr explicit BitFlag(T v)
        : value(v)
    {
    }

    template <typename U>
    requires IsEnum<U>
    constexpr BitFlag(U v)
        : value(detail::bitFlagValue<T>(v))
    {
    }

    constexpr operator bool() const
    {
        return !!value;
    }

    T value;
};

template <typename T>
concept IsBitFlag = requires(T t)
{
    []<typename U>(const BitFlag<U>&){}(t);
};

template <typename T>
requires IsBitFlag<T>
constexpr typename T::Underlying bitFlagValue(typename T::Value v)
{
    return 1 << static_cast<int>(v);
}

template <typename T, typename U = typename T::Value>
requires IsBitFlag<T>
constexpr inline T& operator|=(T& lhs, const U rhs)
{
    lhs.value |= bitFlagValue<T>(rhs);
    return lhs;
}

template <typename T>
requires IsBitFlag<T>
constexpr inline T& operator|=(T& lhs, const T rhs)
{
    lhs.value |= rhs.value;
    return lhs;
}

template <typename T, typename U = typename T::Value>
requires IsBitFlag<T>
constexpr inline T& operator&=(T& lhs, const U rhs)
{
    lhs.value &= bitFlagValue<T>(rhs);
    return lhs;
}

template <typename T>
requires IsBitFlag<T>
constexpr inline T& operator&=(T& lhs, const T rhs)
{
    lhs.value &= rhs.value;
    return lhs;
}

template <typename T, typename U = typename T::Value>
requires IsBitFlag<T>
constexpr inline T& operator^=(T& lhs, const U rhs)
{
    lhs.value ^= bitFlagValue<T>(rhs);
    return lhs;
}

template <typename T>
requires IsBitFlag<T>
constexpr inline T& operator^=(T& lhs, const T rhs)
{
    lhs.value ^= rhs.value;
    return lhs;
}

template <typename T, typename U = typename T::Value>
requires IsBitFlag<T>
constexpr inline T operator|(const T lhs, const U rhs)
{
    return T(lhs.value | bitFlagValue<T>(rhs));
}

template <typename T, typename U = typename T::Value>
requires IsBitFlag<T>
constexpr inline T operator&(const T lhs, const U rhs)
{
    return T(lhs.value & bitFlagValue<T>(rhs));
}

#define DEFINE_BITFLAG(NAME, TYPE, ...) \
    static_assert(std::is_integral_v<TYPE>, "TYPE is required to be an integral type"); \
    struct NAME : ::utils::BitFlag<TYPE> \
    { \
        enum class Value __VA_ARGS__; \
        using enum Value; \
        using BitFlag::BitFlag; \
        constexpr bool operator[](Value v) const { return !!(value & ::utils::bitFlagValue<NAME>(v)); } \
    }; \
    constexpr inline NAME operator|(NAME::Value lhs, NAME::Value rhs) \
    { \
        return NAME(::utils::bitFlagValue<NAME>(lhs) | ::utils::bitFlagValue<NAME>(rhs)); \
    } \
    constexpr inline NAME operator~(NAME::Value v) \
    { \
        return NAME(~(::utils::bitFlagValue<NAME>(v))); \
    } \
    constexpr inline unsigned bitIndex(NAME::Value v) \
    { \
        return static_cast<unsigned>(v); \
    }

}  // namespace utils

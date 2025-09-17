#pragma once

#include <cmath>

#include "utils/inline.hpp"

namespace utils
{

template <typename T>
ALWAYS_INLINE constexpr T clamp(T val, T min, T max)
{
    if (val < min)
    {
        return min;
    }
    else if (val > max)
    {
        return max;
    }
    return val;
}

template <typename T>
ALWAYS_INLINE constexpr T min(T lhs, T rhs)
{
    return lhs > rhs ? rhs : lhs;
}

template <typename T>
ALWAYS_INLINE constexpr T max(T lhs, T rhs)
{
    return lhs > rhs ? lhs : rhs;
}

template <typename T>
ALWAYS_INLINE constexpr T numberOfDigits(T x)
{
    return x > 0 ? (T)std::log10((double)x) + 1 : 1;
}

template <typename T>
constexpr static inline T alignTo(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

}  // namespace utils

#pragma once

#include <cmath>

namespace utils
{

template <typename T>
T clamp(T val, T min, T max)
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
inline T min(T lhs, T rhs)
{
    return lhs > rhs ? rhs : lhs;
}

template <typename T>
inline T max(T lhs, T rhs)
{
    return lhs > rhs ? lhs : rhs;
}

template <typename T>
inline T numberOfDigits(T x)
{
    return x > 0 ? (T)std::log10((double)x) + 1 : 1;
}

}  // namespace utils

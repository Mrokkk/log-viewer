#pragma once

#include <cmath>

namespace utils
{

template <typename T>
struct Clamp { T min, max; };

template <typename T>
Clamp<T> clamp(T min, T max)
{
    return Clamp<T>{min, max};
}

template <typename T>
T operator|(T val, const Clamp<T>& clamp);

extern template int operator|(int val, const Clamp<int>& clamp);
extern template unsigned int operator|(unsigned int val, const Clamp<unsigned int>& clamp);
extern template long operator|(long val, const Clamp<long>& clamp);
extern template unsigned long operator|(unsigned long val, const Clamp<unsigned long>& clamp);

template <typename T>
inline T numberOfDigits(T x)
{
    return x > 0 ? (T)std::log10((double)x) + 1 : 1;
}

}  // namespace utils

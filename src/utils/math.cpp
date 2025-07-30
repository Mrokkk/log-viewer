#include "math.hpp"

namespace utils
{

template <typename T>
T operator|(T val, const Clamp<T>& clamp)
{
    if (val < clamp.min)
    {
        return clamp.min;
    }
    else if (val > clamp.max)
    {
        return clamp.max;
    }
    return val;
}

template int operator|(int val, const Clamp<int>& clamp);
template unsigned int operator|(unsigned int val, const Clamp<unsigned int>& clamp);
template long operator|(long val, const Clamp<long>& clamp);
template unsigned long operator|(unsigned long val, const Clamp<unsigned long>& clamp);

}  // namespace utils

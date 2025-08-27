#pragma once

#include <utility>

namespace utils
{

template <typename T, typename ...Args>
void constructAt(T* data, Args&&... args)
{
    new (data) T(std::forward<Args>(args)...);
}

template <typename T>
void destroyAt(T* data)
{
    data->~T();
}

}  // namespace utils

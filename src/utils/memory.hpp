#pragma once

#include <cstddef>
#include <utility>

#include "utils/inline.hpp"

namespace utils
{

template <typename T, typename ...Args>
ALWAYS_INLINE constexpr void constructAt(T* data, Args&&... args)
{
    new (data) T(std::forward<Args>(args)...);
}

template <typename T>
ALWAYS_INLINE constexpr void destroyAt(T* data)
{
    data->~T();
}

template <typename T, typename U = void*>
ALWAYS_INLINE constexpr T atOffset(U ptr, size_t offset)
{
    return static_cast<T>(static_cast<void*>(static_cast<char*>(ptr) + offset));
}

}  // namespace utils

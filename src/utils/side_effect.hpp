#pragma once

#include <utility>

namespace utils
{

template <typename T>
struct SideEffect { const T sideEffect; };

template <typename T>
SideEffect<T> sideEffect(T&& sideEffect)
{
    return SideEffect<T>{.sideEffect{std::move(sideEffect)}};
}

template <typename T, typename U>
T&& operator|(T&& value, const SideEffect<U>& sideEffect)
{
    sideEffect.sideEffect(value);
    return std::forward<T>(value);
}

}  // namespace utils

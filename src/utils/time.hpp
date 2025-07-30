#pragma once

#include <chrono>

namespace utils
{

template <typename T>
float measureTime(T&& operation)
{
    const auto start = std::chrono::system_clock::now();
    operation();
    const auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed = end - start;
    return elapsed.count();
}

}  // namespace utils

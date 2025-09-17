#pragma once

#include <cstddef>

constexpr static size_t KiB = 1024;
constexpr static size_t MiB = 1024 * KiB;
constexpr static size_t GiB = 1024 * MiB;

consteval size_t operator""_KiB(unsigned long long value)
{
    return KiB * value;
}

consteval size_t operator""_MiB(unsigned long long value)
{
    return MiB * value;
}

consteval size_t operator""_GiB(unsigned long long value)
{
    return GiB * value;
}

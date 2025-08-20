#pragma once

namespace utils
{

struct Immobile
{
    constexpr Immobile() = default;
    Immobile(const Immobile&) = delete;
    Immobile& operator=(const Immobile&) = delete;
    Immobile(Immobile&&) = delete;
    Immobile& operator=(Immobile&&) = delete;
};

}  // namespace utils

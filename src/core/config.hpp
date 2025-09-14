#pragma once

#include <climits>
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

#include "utils/immobile.hpp"
#include "utils/units.hpp"

namespace core
{

template <size_t N, size_t ...T>
struct ChooseNth {};

template <size_t N, size_t T, size_t ...U>
struct ChooseNth<N, T, U...> : ChooseNth<N - 1, U...> {};

template <size_t T, size_t ...U>
struct ChooseNth<0, T, U...>
{
    constexpr static auto value = T;
};

template <typename T, T ...Args>
struct ConfigVariable : utils::Immobile
{
    constexpr static bool hasRange = sizeof...(Args) == 2;

    constexpr ConfigVariable() = default;

    template <typename U>
    requires std::convertible_to<U, T>
    constexpr ConfigVariable(U value)
        : mValue(value)
    {
    }

    consteval static T min()
    {
        return ChooseNth<0, Args...>::value;
    }

    consteval static T max()
    {
        return ChooseNth<1, Args...>::value;
    }

    constexpr operator auto&() const { return mValue; }

    template <typename U>
    requires std::convertible_to<U, T>
    constexpr bool set(U value)
    {
        if constexpr (hasRange)
        {
            if (not validateRange(value)) [[unlikely]]
            {
                return false;
            }
        }
        mValue = std::move(value);
        return true;
    }

    constexpr const T& get() const
    {
        return mValue;
    }

private:
    constexpr static bool validateRange(long value)
    {
        if constexpr (sizeof(T) >= sizeof(long) * (std::is_unsigned_v<T> + 1))
        {
            if (static_cast<T>(value) < min() or
                static_cast<T>(value) > max())
            {
                return false;
            }
        }
        else
        {
            if (static_cast<long long>(value) < static_cast<long long>(min()) or
                static_cast<unsigned long>(value) > static_cast<unsigned long>(max()))
            {
                return false;
            }
        }

        return true;
    }

    T mValue;
};

struct Config
{
    template <size_t ...Args> using Uint8  = ConfigVariable<uint8_t, Args...>;
    template <size_t ...Args> using Size   = ConfigVariable<size_t, Args...>;
                              using Bool   = ConfigVariable<bool>;
                              using String = ConfigVariable<std::string>;

    Uint8<0, UCHAR_MAX> maxThreads = 8;
    Size<0, LONG_MAX>   linesPerThread = 5000000;
    Size<0, LONG_MAX>   bytesPerThread = 1_GiB;
    Bool                showLineNumbers = false;
    Bool                absoluteLineNumbers = false;
    Uint8<0, 16>        scrollJump = 5;
    Uint8<0, 8>         scrollOff = 3;
    Uint8<0, UCHAR_MAX> fastMoveLen = 16;
    Uint8<0, 8>         tabWidth = 4;
    String              lineNumberSeparator = " ";
    String              tabChar = "›";
};

}  // namespace core

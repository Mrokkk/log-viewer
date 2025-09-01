#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>

#include "utils/buffer.hpp"

namespace utils
{

namespace detail
{

constexpr inline void format(Buffer& buf, std::string_view& fmt)
{
    buf << fmt;
}

template <typename First, typename ...Args>
constexpr inline void format(Buffer& buf, std::string_view& fmt, First&& first, Args&&... args)
{
    const auto it = fmt.find("{}");
    buf << fmt.substr(0, it) << std::forward<First>(first);
    fmt.remove_prefix(it + 2);
    detail::format(buf, fmt, std::forward<Args>(args)...);
}

template <size_t got>
void constevalFailure(const char*);

template <typename ...Args>
struct FormatStringChecker
{
    consteval FormatStringChecker(std::string_view str)
    {
        size_t count = 0;
        for (size_t i = 0; i < str.length() - 1; ++i)
        {
            if (str[i] == '{' and str[i + 1] == '}')
            {
                count++;
            }
        }
        if (count != sizeof...(Args))
        {
            constevalFailure<sizeof...(Args)>("Incorrect number of arguments passed");
        }
    }
};

template <typename ...Args>
struct FormatStringImpl
{
    template<typename T>
    requires std::convertible_to<const T&, std::string_view>
    consteval FormatStringImpl(const T& str)
        : mStr(str)
    {
        FormatStringChecker<Args...> check(mStr);
    }

    std::string_view mStr;
};

template <typename ...Args>
using FormatString = FormatStringImpl<std::type_identity_t<Args>...>;

}  // namespace detail

template <typename ...Args>
[[nodiscard]] constexpr Buffer format(detail::FormatString<Args...> fmt, Args&&... args)
{
    Buffer buf;
    detail::format(buf, fmt.mStr, std::forward<Args>(args)...);
    return buf;
}

}  // namespace utils

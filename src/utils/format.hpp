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

template <typename ...Args>
struct FormatStringImpl
{
    template<typename T>
    requires std::convertible_to<const T&, std::string_view>
    consteval FormatStringImpl(const T& str)
        : mStr(str)
    {
        size_t count = 0;
        for (size_t i = 0; i < mStr.length() - 1; ++i)
        {
            if (mStr[i] == '{' and mStr[i + 1] == '}')
            {
                count++;
            }
        }
        if (count != sizeof...(Args))
        {
            failure<sizeof...(Args)>("Incorrect number of arguments passed");
        }
    }

    template <size_t got>
    static void failure(const char*);

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

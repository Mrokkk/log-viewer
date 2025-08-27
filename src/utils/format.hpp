#pragma once

#include <format>
#include <type_traits>

namespace utils
{

template <typename T>
concept IsPointer = std::is_pointer_v<T>
    and not std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, void>
    and not std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>;

}  // namespace utils

template <utils::IsPointer T>
struct std::formatter<T, char>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const T obj, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", static_cast<const void*>(obj));
    }
};

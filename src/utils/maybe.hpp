#pragma once

#include <expected>
#include <type_traits>

namespace utils
{

namespace tag
{
// char added as a workaround for GCC treating empty struct as uninitilized value
constexpr static struct Empty{char c;} empty{0};
}  // namespace tag

template <typename T>
struct Maybe final : std::expected<T, tag::Empty>
{
    using Type = T;
    using Base = std::expected<Type, tag::Empty>;

    constexpr static auto empty = tag::empty;

    constexpr Maybe()
        : Base(std::unexpected(empty))
    {
    }

    constexpr void reset()
    {
        *this = std::unexpected(empty);
    }

    using std::expected<Type, tag::Empty>::expected;
};

template <typename T>
requires std::is_reference_v<T>
struct Maybe<T> final : std::expected<std::remove_reference_t<T>*, tag::Empty>
{
    using Type = std::remove_reference_t<T>*;
    using Base = std::expected<Type, tag::Empty>;

    constexpr static auto empty = tag::empty;

    constexpr Maybe()
        : Base(std::unexpected(empty))
    {
    }

    constexpr Maybe(T value)
        : Base(&value)
    {
    }

    constexpr auto& operator*()
    {
        return *std::expected<Type, tag::Empty>::operator*();
    }

    constexpr auto operator->()
    {
        return std::expected<Type, tag::Empty>::operator*();
    }

    constexpr void reset()
    {
        *this = std::unexpected(empty);
    }

    using std::expected<Type, tag::Empty>::expected;
};

constexpr static auto empty = std::unexpected(tag::empty);

}  // namespace utils

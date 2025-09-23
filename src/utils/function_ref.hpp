#pragma once

#include <utility>
#include <type_traits>

namespace utils
{
namespace detail
{

template <typename Fn, typename ...Args>
requires (not std::is_member_pointer_v<std::decay_t<Fn>>)
constexpr auto invoke(Fn&& f, Args&&... args)
{
    return std::forward<Fn>(f)(std::forward<Args>(args)...);
}

} // namespace detail

template <typename F>
struct FunctionRef;

template <typename R, typename ...Args>
struct FunctionRef<R(Args...)>
{
    constexpr FunctionRef() = default;

    template <typename F>
    requires (not std::is_same_v<std::decay_t<F>, FunctionRef> and std::is_invocable_r_v<R, F&&, Args...>)
    constexpr FunctionRef(F&& f)
        : mData(const_cast<void*>(reinterpret_cast<const void*>(std::addressof(f))))
        , mFunctionPtr(
            [](void* obj, Args... args) -> R
            {
                return detail::invoke(
                    *reinterpret_cast<std::remove_reference_t<F>*>(obj),
                    std::forward<Args>(args)...);
            })
    {
    }

    constexpr FunctionRef(const FunctionRef& rhs) = default;
    constexpr FunctionRef& operator=(const FunctionRef& rhs) = default;

    template <typename F>
    requires std::is_invocable_r_v<R, F&&, Args...>
    constexpr FunctionRef& operator=(F&& f)
    {
        mData = reinterpret_cast<void*>(std::addressof(f));
        mFunctionPtr =
            [](void* obj, Args... args)
            {
                return detail::invoke(
                    *reinterpret_cast<std::remove_reference_t<F>*>(obj),
                    std::forward<Args>(args)...);
            };

        return *this;
    }

    constexpr R operator()(Args... args) const
    {
        return mFunctionPtr(mData, std::forward<Args>(args)...);
    }

private:
    using FunctionPtr = R(*)(void*, Args...);

    void*       mData = nullptr;
    FunctionPtr mFunctionPtr = nullptr;
};

} // namespace tl

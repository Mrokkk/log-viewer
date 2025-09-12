#pragma once

#include <utility>

namespace utils
{

template <typename T>
struct UniquePtr final
{
    constexpr UniquePtr()
        : mPtr(nullptr)
    {
    }

    constexpr UniquePtr(T* ptr)
        : mPtr(ptr)
    {
    }

    constexpr ~UniquePtr()
    {
        reset();
    }

    constexpr UniquePtr(const UniquePtr&)
        = delete("copy of UniquePtr is not allowed");

    constexpr UniquePtr(UniquePtr&& other)
        : mPtr(other.mPtr)
    {
        other.mPtr = nullptr;
    }

    constexpr UniquePtr& operator=(const UniquePtr&)
        = delete("copy of UniquePtr is not allowed");

    constexpr UniquePtr& operator=(UniquePtr&& other)
    {
        reset();
        mPtr = other.mPtr;
        other.mPtr = nullptr;
    }

    constexpr void reset()
    {
        if (mPtr)
        {
            delete mPtr;
            mPtr = nullptr;
        }
    }

    constexpr auto operator->() const { return mPtr; }
    constexpr auto operator->()       { return mPtr; }

    constexpr auto& operator*() const { return *mPtr; }
    constexpr auto& operator*()       { return *mPtr; }

    constexpr auto get() const { return mPtr; }
    constexpr auto get()       { return mPtr; }

    constexpr bool operator==(const T* other) const
    {
        return mPtr == other;
    }

private:
    T* mPtr;
};

template <typename T, typename ...Args>
UniquePtr<T> makeUnique(Args&&... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace utils

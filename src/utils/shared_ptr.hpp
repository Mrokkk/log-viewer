#pragma once

#include <cstdlib>
#include <utility>

#include "utils/memory.hpp"

namespace utils
{

template <typename T>
struct SharedPtr
{
    constexpr SharedPtr()
        : mPtr(nullptr)
        , mRefCount(nullptr)
    {
    }

    constexpr SharedPtr(T* ptr, int* refCount)
        : mPtr(ptr)
        , mRefCount(refCount)
    {
    }

    constexpr SharedPtr(const SharedPtr& other)
    {
        copyFrom(other);
    }

    constexpr SharedPtr(SharedPtr&& other)
    {
        moveFrom(std::move(other));
    }

    constexpr SharedPtr& operator=(const SharedPtr& other)
    {
        copyFrom(other);
        return *this;
    }

    constexpr SharedPtr& operator=(SharedPtr&& other)
    {
        moveFrom(std::move(other));
        return *this;
    }

    constexpr ~SharedPtr()
    {
        destroy();
    }

    constexpr void reset()
    {
        destroy();
        mPtr = nullptr;
        mRefCount = nullptr;
    }

    constexpr auto& operator*()       { return *mPtr; }
    constexpr auto& operator*() const { return *mPtr; }

    constexpr auto operator->()       { return mPtr; }
    constexpr auto operator->() const { return mPtr; }

    constexpr auto get()       { return mPtr; }
    constexpr auto get() const { return mPtr; }

private:
    constexpr void destroy()
    {
        if (mRefCount and --*mRefCount == 0)
        {
            destroyAt(mPtr);
            std::free(mPtr);
        }
    }

    constexpr void copyFrom(const SharedPtr& other)
    {
        mPtr = other.mPtr;
        mRefCount = other.mRefCount;
        if (mRefCount)
        {
            ++*mRefCount;
        }
    }

    constexpr void moveFrom(SharedPtr&& other)
    {
        mPtr = other.mPtr;
        mRefCount = other.mRefCount;
        other.mPtr = nullptr;
        other.mRefCount = nullptr;
    }

    T*   mPtr;
    int* mRefCount;
};

template <typename T, typename ...Args>
SharedPtr<T> makeShared(Args&&... args)
{
    auto allocation = std::malloc(sizeof(T) + sizeof(int));
    auto ptr = atOffset<T*>(allocation, 0);
    auto refCount = atOffset<int*>(allocation, sizeof(T));
    constructAt(ptr, std::forward<Args>(args)...);
    *refCount = 1;
    return SharedPtr<T>(ptr, refCount);
}

}  // namespace utils

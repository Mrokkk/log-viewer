#pragma once

#include <cstddef>
#include <vector>

#include "utils/noncopyable.hpp"

namespace utils
{

template <typename T>
struct RingBuffer final : NonCopyable
{
    constexpr RingBuffer()
        : mStart(0)
        , mSize(0)
        , mCurrent(0)
    {
    }

    constexpr RingBuffer(size_t size)
        : mStart(0)
        , mSize(0)
        , mCurrent(0)
        , mBuffer(size)
    {
    }

    constexpr RingBuffer& pushFront(T value)
    {
        size_t maxSize = mBuffer.size();
        if (static_cast<int>(--mStart) < 0)
        {
            mStart = maxSize - 1;
        }

        if (mSize < maxSize)
        {
            mSize++;
        }
        else if (static_cast<int>(--mCurrent) < 0)
        {
            mCurrent = maxSize - 1;
        }

        mBuffer[mStart] = std::move(value);

        return *this;
    }

    constexpr RingBuffer& pushBack(T value)
    {
        size_t maxSize = mBuffer.size();

        if (mCurrent >= maxSize)
        {
            mCurrent = 0;
        }

        mBuffer[mCurrent++] = std::move(value);

        if (mSize == maxSize)
        {
            if (++mStart >= maxSize)
            {
                mStart = 0;
            }
        }
        else
        {
            ++mSize;
        }

        return *this;
    }

    template <typename U>
    constexpr void forEach(const U& callback) const
    {
        if (mSize == 0) [[unlikely]]
        {
            return;
        }
        if (mCurrent <= mStart)
        {
            for (size_t i = mStart; i < mBuffer.size(); ++i)
            {
                callback(mBuffer[i]);
            }
            for (size_t i = 0; i < mCurrent; ++i)
            {
                callback(mBuffer[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < mCurrent; ++i)
            {
                callback(mBuffer[i]);
            }
        }
    }

    constexpr const T& operator[](size_t i) const
    {
        i += mStart;
        if (i >= mBuffer.size())
        {
            i -= mBuffer.size();
        }
        return mBuffer[i];
    }

    constexpr void clear()
    {
        mCurrent = mSize = mStart = 0;
    }

    constexpr size_t capacity() const
    {
        return mBuffer.size();
    }

    constexpr size_t size() const
    {
        return mSize;
    }

private:
    size_t         mStart;
    size_t         mSize;
    size_t         mCurrent;
    std::vector<T> mBuffer;
};

}  // namespace utils

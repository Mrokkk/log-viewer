#pragma once

#include <cstddef>
#include <vector>

#include "utils/noncopyable.hpp"

namespace utils
{

template <typename T>
struct RingBuffer final : Noncopyable
{
    RingBuffer(size_t size)
        : start_(0)
        , size_(0)
        , current_(0)
        , buffer_(size)
    {
    }

    RingBuffer& pushFront(T value)
    {
        size_t maxSize = buffer_.size();
        if (static_cast<int>(--start_) < 0)
        {
            start_ = maxSize - 1;
        }

        if (size_ < maxSize)
        {
            size_++;
        }
        else if (static_cast<int>(--current_) < 0)
        {
            current_ = maxSize - 1;
        }

        buffer_[start_] = std::move(value);

        return *this;
    }

    RingBuffer& pushBack(T value)
    {
        size_t maxSize = buffer_.size();

        if (current_ >= maxSize)
        {
            current_ = 0;
        }

        buffer_[current_++] = std::move(value);

        if (size_ == maxSize)
        {
            if (++start_ >= maxSize)
            {
                start_ = 0;
            }
        }
        else
        {
            ++size_;
        }

        return *this;
    }

    template <typename U>
    void forEach(const U& callback) const
    {
        if (current_ <= start_)
        {
            for (size_t i = start_; i < buffer_.size(); ++i)
            {
                callback(buffer_[i]);
            }
            for (size_t i = 0; i < current_; ++i)
            {
                callback(buffer_[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < current_; ++i)
            {
                callback(buffer_[i]);
            }
        }
    }

    size_t capacity() const
    {
        return buffer_.size();
    }

    size_t size() const
    {
        return size_;
    }

private:
    size_t         start_;
    size_t         size_;
    size_t         current_;
    std::vector<T> buffer_;
};

}  // namespace utils

#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

#include "utils/enum_traits.hpp"

namespace utils
{

namespace detail
{

template <typename T>
concept PrintablePointer = std::is_pointer_v<T>
    and not std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>;


template <typename T>
struct FormatterInfo
{
};

template <> struct FormatterInfo<int>
{
    constexpr static auto fmt = "%d";
    constexpr static auto len = sizeof("-2147483648");
};

template <> struct FormatterInfo<long>
{
    constexpr static auto fmt = "%ld";
    constexpr static auto len = sizeof("-9223372036854775808");
};

template <> struct FormatterInfo<unsigned char>
{
    constexpr static auto fmt = "%d";
    constexpr static auto len = sizeof("255");
};

template <> struct FormatterInfo<unsigned int>
{
    constexpr static auto fmt = "%u";
    constexpr static auto len = sizeof("4294967295");
};

template <> struct FormatterInfo<unsigned long>
{
    constexpr static auto fmt = "%lu";
    constexpr static auto len = sizeof("18446744073709551615");
};

template <> struct FormatterInfo<float>
{
    constexpr static auto fmt = "%f";
    constexpr static auto len = sizeof("340282346638528859811704183484516925440.000000");
};

struct Pointer;

template <> struct FormatterInfo<Pointer>
{
    constexpr static auto fmt = "%p";
    constexpr static auto len = sizeof("0xffffffffffffffff");
};

template <typename T>
concept KnownFormat = requires (T)
{
    FormatterInfo<T>::fmt;
};

}  // namespace detail

struct Buffer
{
    using value_type = char;
    using size_type = unsigned;

    constexpr static size_type printfTempBuffer = 512;
    constexpr static size_type initialCapacity = 128 - sizeof(char*) - 2 * sizeof(size_type);

    constexpr Buffer()
        : mBuffer(mOwnBuffer)
        , mSize(0)
        , mCapacity(sizeof(mOwnBuffer))
    {
    }

    constexpr Buffer(Buffer&& other)
        : mSize(other.mSize)
        , mCapacity(other.mCapacity)
    {
        moveFrom(std::move(other));
    }

    constexpr ~Buffer()
    {
        if (mBuffer != mOwnBuffer)
        {
            std::free(mBuffer);
        }
    }

    constexpr Buffer& operator=(Buffer&& other)
    {
        if (mBuffer != mOwnBuffer)
        {
            std::free(mBuffer);
        }

        mSize = other.mSize;
        mCapacity = other.mCapacity;

        moveFrom(std::move(other));

        return *this;
    }

    template <typename T>
    requires IsEnum<T>
    constexpr Buffer& operator<<(const T value)
    {
        return operator<<(std::underlying_type_t<T>(value));
    }

    template <typename T>
    requires detail::PrintablePointer<T>
    constexpr Buffer& operator<<(T value)
    {
        using Info = detail::FormatterInfo<detail::Pointer>;
        ensureCapacity(Info::len);
        mSize += snprintf(mBuffer + mSize, Info::len, Info::fmt, value);
        return *this;
    }

    constexpr Buffer& operator<<(const char value)
    {
        ensureCapacity(1);
        mBuffer[mSize++] = value;
        return *this;
    }

    template <typename T>
    requires detail::KnownFormat<T>
    constexpr Buffer& operator<<(const T value)
    {
        using Info = detail::FormatterInfo<T>;
        ensureCapacity(Info::len);
        mSize += snprintf(mBuffer + mSize, Info::len, Info::fmt, value);
        return *this;
    }

    constexpr Buffer& operator<<(const char* text)
    {
        return operator<<(std::string_view(text));
    }

    constexpr Buffer& operator<<(const std::string_view& sv)
    {
        ensureCapacity(sv.size());

        std::memcpy(mBuffer + mSize, sv.data(), sv.size());
        mSize += sv.size();

        return *this;
    }

    constexpr Buffer& operator<<(const std::string& s)
    {
        ensureCapacity(s.size());

        std::memcpy(mBuffer + mSize, s.data(), s.size());
        mSize += s.size();

        return *this;
    }

    constexpr Buffer& operator<<(const bool value)
    {
        const std::string_view sv = value
            ? "true"
            : "false";

        ensureCapacity(sv.size());

        std::memcpy(mBuffer + mSize, sv.data(), sv.size());
        mSize += sv.size();

        return *this;
    }

    constexpr void clear()
    {
        mSize = 0;
    }

    constexpr const std::string_view view() const
    {
        return std::string_view(mBuffer, mSize);
    }

    constexpr const char* buffer() const
    {
        return mBuffer;
    }

    std::string str() const
    {
        return std::string(mBuffer, mSize);
    }

    constexpr size_type length() const
    {
        return mSize;
    }

    constexpr size_type capacity() const
    {
        return mCapacity;
    }

    constexpr operator std::string_view() const
    {
        return view();
    }

    constexpr Buffer& printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)))
    {
        char buffer[printfTempBuffer];
        va_list args;
        va_start(args, fmt);
        auto len = vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        ensureCapacity(len);
        std::memcpy(mBuffer + mSize, buffer, len);
        mSize += len;
        return *this;
    }

private:
    constexpr void moveFrom(Buffer&& other)
    {
        if (other.mBuffer == other.mOwnBuffer)
        {
            std::memcpy(mOwnBuffer, other.mOwnBuffer, mSize);
            mBuffer = mOwnBuffer;
        }
        else
        {
            mBuffer = other.mBuffer;
        }

        other.mBuffer = other.mOwnBuffer;
        other.mCapacity = initialCapacity;
        other.mSize = 0;
    }

    constexpr static size_type max(size_type a, size_type b)
    {
        return a > b ? a : b;
    }

    constexpr void ensureCapacity(size_type additionalSize)
    {
        size_type requiredSize = mSize + additionalSize;
        if (requiredSize > mCapacity)
        {
            size_type newCapacity = max(mCapacity * 2, requiredSize);
            if (mBuffer == mOwnBuffer)
            {
                mBuffer = static_cast<char*>(std::malloc(newCapacity));
                memcpy(mBuffer, mOwnBuffer, mSize);
            }
            else
            {
                mBuffer = static_cast<char*>(std::realloc(mBuffer, newCapacity));
            }
            mCapacity = newCapacity;
        }
    }

    char*     mBuffer;
    size_type mSize;
    size_type mCapacity;
    char      mOwnBuffer[initialCapacity];
};

}  // namespace utils

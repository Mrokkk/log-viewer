#pragma once

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

#include "utils/bitflag.hpp"
#include "utils/enum_traits.hpp"

namespace utils
{

namespace detail
{

constexpr static auto startFmtChar = '%';
constexpr static auto showbaseChar = '#';
constexpr static auto paddingChar = '*';

template <typename T>
concept PrintablePointer = std::is_pointer_v<T>
    and not std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>;

struct Pointer final
{
    template <typename T>
    requires PrintablePointer<T>
    constexpr Pointer(T ptr)
        : mPtr(ptr)
    {
    }

    constexpr operator const void*() const
    {
        return mPtr;
    }

private:
    const void* mPtr;
};

template <typename T>
struct FormatterInfo final
{
};

template <> struct FormatterInfo<char>
{
    constexpr static auto fmt = "%c";
    constexpr static auto fmtChar = 'c';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = sizeof("0377");
};

template <> struct FormatterInfo<int>
{
    constexpr static auto fmt = "%d";
    constexpr static auto fmtChar = 'd';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = sizeof("017777777777");
};

template <> struct FormatterInfo<long>
{
    constexpr static auto fmt = "%ld";
    constexpr static auto fmtChar = 'd';
    constexpr static auto modifierChar = 'l';
    constexpr static auto len = sizeof("01777777777777777777777");
};

template <> struct FormatterInfo<unsigned char>
{
    constexpr static auto fmt = "%d";
    constexpr static auto fmtChar = 'd';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = FormatterInfo<char>::len;
};

template <> struct FormatterInfo<unsigned int>
{
    constexpr static auto fmt = "%u";
    constexpr static auto fmtChar = 'u';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = FormatterInfo<int>::len;
};

template <> struct FormatterInfo<unsigned long>
{
    constexpr static auto fmt = "%lu";
    constexpr static auto fmtChar = 'u';
    constexpr static auto modifierChar = 'l';
    constexpr static auto len = FormatterInfo<long>::len;
};

template <> struct FormatterInfo<float>
{
    constexpr static auto fmt = "%f";
    constexpr static auto fmtChar = 'f';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = sizeof("340282346638528859811704183484516925440.000000");
};

template <> struct FormatterInfo<Pointer>
{
    constexpr static auto fmt = "%p";
    constexpr static auto fmtChar = 'p';
    constexpr static auto modifierChar = '\0';
    constexpr static auto len = 2 * sizeof(void*) + 3;
};

template <typename T>
constexpr inline T native(T&& val)
{
    return std::forward<T>(val);
}

constexpr inline const void* native(Pointer p)
{
    return p;
}

template <typename T>
concept HasKnownFormat = requires (T)
{
    FormatterInfo<T>::fmt;
};

DEFINE_BITFLAG(Flags, char,
{
    hex,
    oct,
    showbase,
});

template <typename T>
struct Manipulator final
{
    template <typename U>
    requires std::convertible_to<T, U>
    constexpr Manipulator<U> convert() const
    {
        return Manipulator<U>{
            .flags = flags,
            .precision = precision,
            .padding = padding,
            .value = static_cast<U>(value),
        };
    }

    template <typename U>
    constexpr Manipulator<U> convert(U&& value) const
    {
        return Manipulator<U>{
            .flags = flags,
            .precision = precision,
            .padding = padding,
            .value = std::forward<U>(value),
        };
    }

    template <typename U>
    requires (not std::convertible_to<T, U>)
    constexpr Manipulator<U> convert() const
        = delete("types are not convertible");

    Flags   flags;
    uint8_t precision = 6;
    int     padding = 0;
    T       value;
};

template <typename T, template <typename> typename U>
inline constexpr bool IsInstanceOf = std::false_type{};

template <template <typename> typename U, typename V>
inline constexpr bool IsInstanceOf<U<V>,U> = std::true_type{};

template <typename T>
concept IsManipulator = IsInstanceOf<T, Manipulator>;

struct ShowBase final {};
struct NoShowBase final {};

}  // namespace detail

struct IntegralBase final
{
    constexpr explicit IntegralBase(detail::Flags f)
        : flags(f)
    {
    }

    constexpr IntegralBase operator()(detail::ShowBase) const
    {
        return IntegralBase(flags | detail::Flags::showbase);
    }

    constexpr IntegralBase operator()(detail::NoShowBase) const
    {
        return IntegralBase(flags & ~detail::Flags::showbase);
    }

    const detail::Flags flags;
};

struct Precision final
{
    uint8_t value;
};

struct Padding final
{
    int value;
};

constexpr static detail::ShowBase showbase;
constexpr static detail::NoShowBase noshowbase;

constexpr static IntegralBase hex(detail::Flags::hex);
constexpr static IntegralBase oct(detail::Flags::oct);

constexpr static Precision precision(uint8_t val)
{
    return {val};
}

constexpr inline Padding leftPadding(int val)
{
    return {val};
}

constexpr inline Padding rightPadding(int val)
{
    return {-val};
}

template <typename T>
requires std::integral<T>
constexpr inline detail::Manipulator<T> operator|(T value, IntegralBase base)
{
    return detail::Manipulator<T>{
        .flags = base.flags,
        .padding = 0,
        .value = value,
    };
}

template <typename T>
requires (not std::integral<T>)
constexpr inline detail::Manipulator<T> operator|(T, IntegralBase)
    = delete("manipulator is possible only for integral types");

template <typename T>
requires std::integral<T>
constexpr inline detail::Manipulator<T> operator|(detail::Manipulator<T>&& m, IntegralBase base)
{
    m.flags &= ~(detail::Flags::hex | detail::Flags::oct | detail::Flags::showbase);
    m.flags |= base.flags;
    return m;
}

template <typename T>
requires (not std::integral<T>)
constexpr inline detail::Manipulator<T> operator|(detail::Manipulator<T>&&, IntegralBase)
    = delete("manipulator is possible only for integral types");

template <typename T>
requires std::floating_point<T>
constexpr inline detail::Manipulator<T> operator|(T value, Precision p)
{
    return detail::Manipulator<T>{
        .flags{},
        .precision = p.value,
        .value = value,
    };
}

template <typename T>
requires (not std::floating_point<T>)
constexpr inline detail::Manipulator<T> operator|(T, Precision)
    = delete("manipulator is possible only for floating-point types");

template <typename T>
requires std::floating_point<T>
constexpr inline detail::Manipulator<T> operator|(detail::Manipulator<T>&& m, Precision p)
{
    m.precision = p.value;
    return m;
}

template <typename T>
requires (not std::floating_point<T>)
constexpr inline detail::Manipulator<T> operator|(detail::Manipulator<T>&&, Precision)
    = delete("manipulator is possible only  for floating-point types");

template <typename T>
requires (not detail::IsManipulator<T>)
constexpr inline detail::Manipulator<T> operator|(T value, Padding pad)
{
    return detail::Manipulator<T>{
        .flags{},
        .padding = pad.value,
        .value = std::move(value),
    };
}

template <typename T>
constexpr inline detail::Manipulator<T> operator|(detail::Manipulator<T>&& m, Padding pad)
{
    m.padding = pad.value;
    return m;
}

struct Buffer final
{
    using value_type = char;
    using size_type = unsigned;

    constexpr static size_type initialCapacity = 128 - sizeof(char*) - 2 * sizeof(size_type);
    constexpr static size_type allocationFactor = 2;

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

    constexpr Buffer& operator<<(const char value)
    {
        ensureCapacity(1);
        mBuffer[mSize++] = value;
        return *this;
    }

    template <typename T>
    requires detail::HasKnownFormat<T>
    constexpr Buffer& operator<<(const T value)
    {
        using Info = detail::FormatterInfo<T>;
        ensureCapacity(Info::len);
        mSize += sprintf(mBuffer + mSize, Info::fmt, detail::native(value));
        return *this;
    }

    template <typename T>
    requires detail::HasKnownFormat<T>
    constexpr Buffer& operator<<(detail::Manipulator<T>&& m)
    {
        using Info = detail::FormatterInfo<T>;

        int i = 0;
        char fmt[16];

        const auto appendFmt =
            [&fmt, &i](char c, bool condition = true) constexpr
            {
                if (condition)
                {
                    fmt[i++] = c;
                }
            };

        appendFmt(detail::startFmtChar);
        appendFmt(detail::showbaseChar, m.flags[detail::Flags::showbase]);
        appendFmt(detail::paddingChar);
        appendFmt(Info::modifierChar, Info::modifierChar != '\0');

        if constexpr (std::is_floating_point_v<T>)
        {
            appendFmt('.');
            i += snprintf(fmt + i, sizeof(fmt) - i, "%u", m.precision);
            appendFmt(Info::fmtChar);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            switch (m.flags & (detail::Flags::hex | detail::Flags::oct))
            {
                case detail::Flags(detail::Flags::hex):
                    appendFmt('x');
                    break;
                case detail::Flags(detail::Flags::oct):
                    appendFmt('o');
                    break;
                default:
                    appendFmt(Info::fmtChar);
                    break;
            }
        }
        else
        {
            appendFmt(Info::fmtChar);
        }

        appendFmt('\0');

        const size_type requiredSize = Info::len + std::abs(m.padding) + m.precision - 6;
        ensureCapacity(requiredSize);

        mSize += sprintf(mBuffer + mSize, fmt, m.padding, m.value);

        return *this;
    }

    template <typename T>
    requires std::convertible_to<T, std::string_view>
    constexpr Buffer& operator<<(T&& string)
    {
        std::string_view sv(std::forward<T>(string));
        ensureCapacity(sv.size());
        std::memcpy(mBuffer + mSize, sv.data(), sv.size());
        mSize += sv.size();
        return *this;
    }

    template <typename T>
    requires std::convertible_to<T, std::string_view>
    constexpr Buffer& operator<<(detail::Manipulator<T>&& m)
    {
        std::string_view sv(std::forward<T>(m.value));

        const size_type absPadding = std::abs(m.padding);
        size_type requiredSize = sv.size() + absPadding;
        size_type leftPadding = m.padding > 0 and absPadding > sv.size() ? absPadding - sv.size() : 0;
        size_type rightPadding = m.padding < 0 and absPadding > sv.size() ? absPadding - sv.size() : 0;

        ensureCapacity(requiredSize);

        for (size_type i = 0; i < leftPadding; mBuffer[mSize++] = ' ', ++i);

        std::memcpy(mBuffer + mSize, sv.data(), sv.size());
        mSize += sv.size();

        for (size_type i = 0; i < rightPadding; mBuffer[mSize++] = ' ', ++i);

        return *this;
    }

    template <typename T>
    requires IsEnum<T>
    constexpr Buffer& operator<<(const T value)
    {
        return operator<<(std::underlying_type_t<T>(value));
    }

    template <typename T>
    requires IsEnum<T>
    constexpr Buffer& operator<<(detail::Manipulator<T>&& m)
    {
        return operator<<(m.template convert<std::underlying_type_t<T>>());
    }

    template <typename T>
    requires detail::PrintablePointer<T>
    constexpr Buffer& operator<<(const T value)
    {
        return operator<<(detail::Pointer(value));
    }

    template <typename T>
    requires detail::PrintablePointer<T>
    constexpr Buffer& operator<<(detail::Manipulator<T>&& m)
    {
        return operator<<(m.template convert<detail::Pointer>());
    }

    constexpr Buffer& operator<<(const std::string& string)
    {
        return operator<<(std::string_view(string));
    }

    template <typename T>
    requires (std::convertible_to<T, std::string> and not std::convertible_to<T, std::string_view>)
    constexpr Buffer& operator<<(detail::Manipulator<T>&& m)
    {
        return operator<<(m.convert(std::string_view(static_cast<std::string>(m.value))));
    }

    constexpr Buffer& operator<<(const bool value)
    {
        constexpr static std::string_view map[] = {
            "false",
            "true"
        };

        return operator<<(map[static_cast<int>(value)]);
    }

    constexpr void clear()
    {
        mSize = 0;
    }

    constexpr std::string_view view() const
    {
        return std::string_view(mBuffer, mSize);
    }

    constexpr const char* buffer() const
    {
        return mBuffer;
    }

    constexpr std::string str() const
    {
        return std::string(mBuffer, mSize);
    }

    constexpr size_type length() const
    {
        return mSize;
    }

    constexpr size_type size() const
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

    constexpr Buffer& write(const char* data, size_type count)
    {
        return operator<<(std::string_view(data, count));
    }

    constexpr const char* begin() const
    {
        return mBuffer;
    }

    constexpr const char* end() const
    {
        return mBuffer + mSize;
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
            size_type newCapacity = max(mCapacity * allocationFactor, requiredSize);
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

namespace detail
{

template <typename T>
concept NativelyPrintable = requires (T t)
{
    Buffer().operator<<(t);
};

template <typename T>
requires (not NativelyPrintable<T>)
constexpr Buffer& operator<<(Buffer& buf, Manipulator<T>&& m)
{
    Buffer tmp;
    tmp << m.value;
    return buf.operator<<(m.convert(tmp.view()));
}

}  // namespace detail

}  // namespace utils

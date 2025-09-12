#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace core
{

struct Utf8 final
{
    struct Invalid final
    {
        constexpr Invalid(uint8_t b)
            : byte(b)
        {
        }

        uint8_t byte;
    };

    constexpr static Utf8 parse(const std::string_view& c)
    {
        if ((c[0] & 0b1000'0000) == 0b0000'0000) [[likely]]
        {
            return Utf8(c[0]);
        }
        else if ((c[0] & 0b1110'0000) == 0b1100'0000 and c.size() > 1)
        {
            return Utf8(c[0], c[1]);
        }
        else if ((c[0] & 0b1111'0000) == 0b1110'0000 and c.size() > 2)
        {
            return Utf8(c[0], c[1], c[2]);
        }
        else if ((c[0] & 0b1111'1000) == 0b1111'0000 and c.size() > 3)
        {
            return Utf8(c[0], c[1], c[2], c[3]);
        }
        else
        {
            return Utf8(Invalid(c[0]));
        }
    }

    constexpr operator uint32_t() const
    {
        return value;
    }

    constexpr uint32_t operator[](size_t i) const
    {
        return bytes[i];
    }

    const uint8_t len;
    const bool    invalid;
    union
    {
        const uint8_t  bytes[4];
        const uint32_t value;
    };

private:
    constexpr Utf8(Invalid b)
        : len(1)
        , invalid(true)
        , bytes{b.byte}
    {
    }

    template <typename ...Args>
    constexpr Utf8(Args... args)
        : len(sizeof...(Args))
        , invalid(false)
        , bytes{uint8_t(args)...}
    {
    }
};

}  // namespace core

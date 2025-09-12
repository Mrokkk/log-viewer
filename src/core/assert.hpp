#pragma once

#include <cassert> // IWYU pragma: export
#include <cstddef>
#include <string_view>

namespace core
{

#ifdef assert
#undef assert
#endif

#define GET_3RD_ARG(_1, _2, _3, ...) _3

#define MACRO_CHOOSER_2(m1, m2, ...) \
    GET_3RD_ARG(__VA_ARGS__, m2, m1)

#define REAL_VAR_MACRO_2(m1, m2, ...) \
    MACRO_CHOOSER_2(m1, m2, __VA_ARGS__)(__VA_ARGS__)

#define ASSERT_1(expr) \
    ({ \
        if (not static_cast<bool>(expr)) [[unlikely]] \
        { \
            ::core::detail::assertionFailed(#expr, __FILE__, __LINE__, __func__); \
        } \
        void(0); \
     })

#define ASSERT_2(expr, message) \
    ({ \
        if (not static_cast<bool>(expr)) [[unlikely]] \
        { \
            ::core::detail::assertionFailed(message, __FILE__, __LINE__, __func__); \
        } \
        void(0); \
    })

#ifndef NDEBUG
#define assert(...) REAL_VAR_MACRO_2(ASSERT_1, ASSERT_2, __VA_ARGS__)
#else
#define assert(...) void(0)
#endif

namespace detail
{

[[noreturn]] void assertionFailed(std::string_view message, const char* file, size_t line, const char* func);

}  // namespace detail

}  // namespace core

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace utils
{

using Strings = std::vector<std::string>;
using StringViews = std::vector<std::string_view>;
using StringRefs = std::vector<const std::string*>;

template <typename T> struct To {};

struct SplitBy { const char* delimiters; };
struct ReadTextWithLimit { size_t fileSizeLimit; };

struct ReadTextWithoutLimit
{
    constexpr static ReadTextWithLimit operator()(size_t limit)
    {
        return ReadTextWithLimit{limit};
    }
};

static constexpr struct IsNumeric {} isNumeric;
static constexpr struct LowerCase {} lowerCase;
static constexpr ReadTextWithoutLimit readText;

template <typename T> static constexpr To<T> to;

SplitBy splitBy(const char* delimiter);

Strings     operator|(const std::string& text, const SplitBy& splitBy);
Strings     operator|(const char* text, const SplitBy& splitBy);
std::string operator|(const std::string& path, const ReadTextWithoutLimit&);
std::string operator|(const std::string& path, const ReadTextWithLimit& readText);
bool        operator|(const std::string& text, const IsNumeric&);
bool        operator|(const char* text, const IsNumeric&);
std::string operator|(std::string text, const LowerCase&);
std::string operator|(const std::string_view& text, const LowerCase&);

template <typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

namespace detail
{
long convert(const std::string_view& text);
}  // namespace detail

template <IsArithmetic T>
T operator|(const std::string_view& text, const To<T>&)
{
    return static_cast<T>(detail::convert(text));
}

}  // namespace utils

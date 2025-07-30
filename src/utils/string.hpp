#pragma once

#include <charconv>
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

static constexpr struct ReadText {}  readText;
static constexpr struct IsNumeric {} isNumeric;

template <typename T> static constexpr To<T> to;

SplitBy splitBy(const char* delimiter);

Strings     operator|(const std::string& text, const SplitBy& splitBy);
Strings     operator|(const char* text, const SplitBy& splitBy);
std::string operator|(const std::string& path, const ReadText&);
bool        operator|(const std::string& text, const IsNumeric&);
bool        operator|(const char* text, const IsNumeric&);

template <typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template <IsArithmetic T, typename U>
long operator|(const U& text, const To<T>&)
{
    T value{};
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

}  // namespace utils

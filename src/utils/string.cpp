#include "string.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string_view>

namespace utils
{

SplitBy splitBy(const char* delimiter)
{
    return SplitBy{.delimiters = delimiter};
}

std::string operator|(const std::string& path, const ReadText&)
{
    std::ifstream file(path);

    if (file.fail()) [[unlikely]]
    {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Strings operator|(const std::string& text, const SplitBy& splitBy)
{
    return std::views::split(std::string_view(text), std::string_view(splitBy.delimiters))
        | std::ranges::views::filter([](const auto& s){ return not s.empty(); })
        | std::ranges::to<Strings>();
}

Strings operator|(const char* text, const SplitBy& splitBy)
{
    return std::views::split(std::string_view(text), std::string_view(splitBy.delimiters))
        | std::ranges::views::filter([](const auto& s){ return not s.empty(); })
        | std::ranges::to<Strings>();
}

bool operator|(const std::string& text, const IsNumeric&)
{
    if (text.empty())
    {
        return false;
    }
    return std::all_of(
        text.begin(), text.end(),
        [](const auto c){ return std::isdigit(c) or c == '-' or c == '+'; });
}

bool operator|(const char* text, const IsNumeric&)
{
    const auto len = std::strlen(text);
    if (len == 0)
    {
        return false;
    }
    return std::all_of(
        text, text + len,
        [](const auto c){ return std::isdigit(c); });
}

std::string operator|(std::string text, const LowerCase&)
{
    std::transform(
        text.begin(), text.end(),
        text.begin(),
        [](char c){ return std::tolower(c); });
    return text;
}

std::string operator|(const std::string_view& text, const LowerCase&)
{
    return operator|(std::string(text), lowerCase);
}

namespace detail
{

long convert(const std::string_view& text)
{
    long value{};
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

}  // namespace detail

}  // namespace utils

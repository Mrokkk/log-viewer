#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "utils/fwd.hpp"

namespace core
{

struct Token
{
    enum class Type
    {
        comment,
        stringLiteral,
        intLiteral,
        booleanLiteral,
        exclamation,
        slash,
        pipe,
        dot,
        add,
        sub,
        dollar,
        leftParenthesis,
        rightParenthesis,
        leftBracket,
        rightBracket,
        percent,
        identifier,
        semicolon,
        newline,
        whitespace,
        end,
    };

    Type             type;
    std::string_view value;
};

using Tokens = std::vector<Token>;

std::expected<Tokens, std::string> parse(const std::string& code);

utils::Buffer& operator<<(utils::Buffer& buf, const Token::Type type);
utils::Buffer& operator<<(utils::Buffer& buf, const Token& token);

}  // namespace core

#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

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

std::ostream& operator<<(std::ostream& os, const Token::Type type);
std::ostream& operator<<(std::ostream& os, const Token& token);

}  // namespace core

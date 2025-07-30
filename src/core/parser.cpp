#include "parser.hpp"

#include <cctype>
#include <expected>
#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace core
{

std::ostream& operator<<(std::ostream& os, const Token::Type type)
{
#define TOKEN_TYPE_PRINT(type) \
    case Token::Type::type: \
        return os << #type
    switch (type)
    {
        TOKEN_TYPE_PRINT(comment);
        TOKEN_TYPE_PRINT(stringLiteral);
        TOKEN_TYPE_PRINT(intLiteral);
        TOKEN_TYPE_PRINT(booleanLiteral);
        TOKEN_TYPE_PRINT(exclamation);
        TOKEN_TYPE_PRINT(leftParenthesis);
        TOKEN_TYPE_PRINT(rightParenthesis);
        TOKEN_TYPE_PRINT(leftBracket);
        TOKEN_TYPE_PRINT(rightBracket);
        TOKEN_TYPE_PRINT(percent);
        TOKEN_TYPE_PRINT(variableReference);
        TOKEN_TYPE_PRINT(identifier);
        TOKEN_TYPE_PRINT(semicolon);
        TOKEN_TYPE_PRINT(newline);
        TOKEN_TYPE_PRINT(end);
    }
    return os << "unknown token{" << static_cast<int>(type) << '}';
}

std::ostream& operator<<(std::ostream& os, const Token& token)
{
    return os << token.type << '{' << token.value << '}';
}

struct LexerState
{
    Tokens            tokens;
    const char*       current;
    std::stringstream error;
};

using TokenHandler = std::function<bool(LexerState&)>;

struct TokenMapping
{
    Token::Type         type;
    TokenHandler        handler;
};

using Tokenhandlers = std::vector<TokenHandler>;

static char peek(LexerState& state)
{
    return *state.current;
}

static char peekNext(LexerState& state)
{
    return *(state.current + 1);
}

static void advance(LexerState& state)
{
    ++state.current;
}

static const char* current(LexerState& state)
{
    return state.current;
}

static bool isSpace(char c)
{
    return c == ' ' or c == '\t';
}

static std::string_view peekWord(LexerState& state)
{
    auto it = current(state);
    while (*it)
    {
        if (isSpace(*it))
        {
            break;
        }
        ++it;
    }
    return std::string_view(current(state), it);
}

static void advance(std::string_view view, LexerState& state)
{
    state.current = view.data() + view.size();
}

static void addToken(Token::Type type, const char* start, const char* end, LexerState& state)
{
    state.tokens.push_back(
        Token{
            .type = type,
            .value = std::string_view(start, end)
        });
}

static void addToken(Token::Type type, const char* start, LexerState& state)
{
    state.tokens.push_back(
        Token{
            .type = type,
            .value = std::string_view(start, current(state))
        });
}

static bool stringLiteral(LexerState& state)
{
    if (peek(state) != '\"')
    {
        return false;
    }

    advance(state);

    const auto start = current(state);

    bool foundEnd{false};

    while (const auto c = peek(state))
    {
        if (c == '\"')
        {
            advance(state);
            foundEnd = true;
            break;
        }
        else if (c == '\\' and peekNext(state) == '\"')
        {
            advance(state);
        }
        advance(state);
    }

    // FIXME: return some error
    if (not foundEnd)
    {
        return false;
    }

    addToken(Token::Type::stringLiteral, start, current(state) - 1, state);

    return true;
}

static bool intLiteral(LexerState& state)
{
    auto start = current(state);

    const auto c = peek(state);

    if (std::isdigit(c))
    {
        advance(state);
    }
    else if ((c == '+' or c == '-') and std::isdigit(peekNext(state)))
    {
        // FIXME: + and - should not be handled here and certainly
        // should not be removed from token
        advance(state);
        if (c == '+')
        {
            start = current(state);
        }
    }
    else
    {
        return false;
    }

    while (const auto c = peek(state))
    {
        if (not isdigit(c))
        {
            break;
        }
        advance(state);
    }

    addToken(Token::Type::intLiteral, start, state);

    return true;
}

static bool comment(LexerState& state)
{
    const auto start = current(state);

    if (peek(state) != '#')
    {
        return false;
    }

    advance(state);

    while (const auto c = peek(state))
    {
        if (c == '\n')
        {
            break;
        }
        advance(state);
    }

    addToken(Token::Type::comment, start, state);

    return true;
}

static bool variableReferenceHandle(LexerState& state)
{
    if (peek(state) != '$' or not std::isalpha(peekNext(state)))
    {
        return false;
    }

    advance(state);

    const auto start = current(state);

    while (const auto c = peek(state))
    {
        if (not std::isalnum(c))
        {
            break;
        }
        advance(state);
    }

    addToken(Token::Type::variableReference, start, state);

    return true;
}

static bool identifier(LexerState& state)
{
    if (not std::isalpha(peek(state)))
    {
        return false;
    }

    const auto start = current(state);

    advance(state);

    while (const auto c = peek(state))
    {
        if (not std::isalnum(c))
        {
            break;
        }
        advance(state);
    }

    addToken(Token::Type::identifier, start, state);

    return true;
}

static TokenHandler singleChar(const char character, Token::Type type)
{
    return
        [character, type](LexerState& state)
        {
            const auto start = current(state);
            if (peek(state) != character)
            {
                return false;
            }
            advance(state);
            addToken(type, start, state);
            return true;
        };
}

static bool space(LexerState& state)
{
    const auto c = peek(state);

    if (not isSpace(c))
    {
        return false;
    }

    while (const auto c = peek(state))
    {
        if (not isSpace(c))
        {
            break;
        }
        advance(state);
    }
    return true;
}

static bool booleanLiteral(LexerState& state)
{
    auto word = peekWord(state);
    if (word == "true" or word == "false")
    {
        advance(word, state);
        addToken(Token::Type::booleanLiteral, word.data(), current(state), state);
        return true;
    }
    return false;
}

static const Tokenhandlers handlers = {
    space,
    comment,
    singleChar('\n', Token::Type::newline),
    singleChar(';', Token::Type::semicolon),
    singleChar('%', Token::Type::percent),
    singleChar('!', Token::Type::exclamation),
    singleChar('(', Token::Type::leftParenthesis),
    singleChar(')', Token::Type::rightParenthesis),
    singleChar('{', Token::Type::leftBracket),
    singleChar('}', Token::Type::rightBracket),
    variableReferenceHandle,
    intLiteral,
    stringLiteral,
    booleanLiteral,
    identifier,
};

std::expected<Tokens, std::string> parse(const std::string& code)
{
    LexerState state{.current = code.c_str()};

    while (peek(state))
    {
        bool found{false};

        for (const auto& handler : handlers)
        {
            if (handler(state))
            {
                found = true;
                break;
            }
        }

        if (not found) [[unlikely]]
        {
            state.error << "unknown token at \"" << state.current << "\"\n";
            return std::unexpected(state.error.str());
        }
    }

    state.tokens.emplace_back(Token{.type = Token::Type::end});

    return std::move(state.tokens);
}

}  // namespace core

#include <ostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/interpreter/lexer.hpp"
#include "utils/buffer.hpp"

using namespace core::interpreter;

namespace core::interpreter
{

std::ostream& operator<<(std::ostream& os, const Token::Type type)
{
    utils::Buffer buf;
    buf << type;
    return os << buf.view();
}

}  // namespace core::interpreter

#define ASSERT_NEXT_TOKEN(TYPE, VALUE) \
    do \
    { \
        if (tokenIt == result.value().end()) \
        { \
            FAIL() << "no more tokens, while expected token: " << Token::Type::TYPE; \
        } \
        ASSERT_EQ(tokenIt->type, Token::Type::TYPE); \
        ASSERT_EQ(tokenIt->value, VALUE); \
        ++tokenIt; \
    } \
    while (0)

TEST(ParserTests, canParse)
{
    std::string value =
        "command!\"hello\"  +21-32 # some comment\n"
        "otherCommand\"world\"  $path   # some other comment\n"
        "{ anotherCommand arg;;; }\n"
        "true truev2 false falseb -test false\n"
        "false";

    auto result = parse(value);
    ASSERT_TRUE(result);

    auto tokenIt = result.value().begin();

    ASSERT_NEXT_TOKEN(identifier, "command");
    ASSERT_NEXT_TOKEN(exclamation, "!");
    ASSERT_NEXT_TOKEN(stringLiteral, "hello");
    ASSERT_NEXT_TOKEN(whitespace, "  ");
    ASSERT_NEXT_TOKEN(add, "+");
    ASSERT_NEXT_TOKEN(intLiteral, "21");
    ASSERT_NEXT_TOKEN(sub, "-");
    ASSERT_NEXT_TOKEN(intLiteral, "32");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(comment, "# some comment");
    ASSERT_NEXT_TOKEN(newline, "\n");
    ASSERT_NEXT_TOKEN(identifier, "otherCommand");
    ASSERT_NEXT_TOKEN(stringLiteral, "world");
    ASSERT_NEXT_TOKEN(whitespace, "  ");
    ASSERT_NEXT_TOKEN(dollar, "$");
    ASSERT_NEXT_TOKEN(identifier, "path");
    ASSERT_NEXT_TOKEN(whitespace, "   ");
    ASSERT_NEXT_TOKEN(comment, "# some other comment");
    ASSERT_NEXT_TOKEN(newline, "\n");
    ASSERT_NEXT_TOKEN(leftBracket, "{");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(identifier, "anotherCommand");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(identifier, "arg");
    ASSERT_NEXT_TOKEN(semicolon, ";");
    ASSERT_NEXT_TOKEN(semicolon, ";");
    ASSERT_NEXT_TOKEN(semicolon, ";");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(rightBracket, "}");
    ASSERT_NEXT_TOKEN(newline, "\n");
    ASSERT_NEXT_TOKEN(booleanLiteral, "true");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(identifier, "truev2");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(booleanLiteral, "false");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(identifier, "falseb");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(sub, "-");
    ASSERT_NEXT_TOKEN(identifier, "test");
    ASSERT_NEXT_TOKEN(whitespace, " ");
    ASSERT_NEXT_TOKEN(booleanLiteral, "false");
    ASSERT_NEXT_TOKEN(newline, "\n");
    ASSERT_NEXT_TOKEN(booleanLiteral, "false");
    ASSERT_NEXT_TOKEN(end, "");
}

TEST(ParserTests, canDetectErrors)
{
    auto result = parse(",");
    ASSERT_FALSE(result);

    result = parse("\"skksjs");
    ASSERT_FALSE(result);

    result = parse("&");
    ASSERT_FALSE(result);

    result = parse("-:");
    ASSERT_FALSE(result);
}

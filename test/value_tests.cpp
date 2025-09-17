#include <gtest/gtest.h>

#include "core/interpreter/value.hpp"

using namespace core::interpreter;

TEST(ValueTests, integer)
{
    Value o(9283847l);
    ASSERT_EQ(o.type(), Value::Type::integer);

    auto integer = o.integer();

    ASSERT_TRUE(integer);
    ASSERT_EQ(integer.value(), 9283847l);

    ASSERT_FALSE(o.string());
    ASSERT_FALSE(o.boolean());

    Value copy(o);

    ASSERT_EQ(copy.type(), Value::Type::integer);

    ASSERT_TRUE(copy.integer());
    ASSERT_FALSE(copy.boolean());
    ASSERT_FALSE(copy.string());
    ASSERT_FALSE(copy.object());
    ASSERT_EQ(copy.integer(), 9283847l);

    Value moved(std::move(copy));

    ASSERT_EQ(copy.type(), Value::Type::null);
    ASSERT_EQ(moved.type(), Value::Type::integer);
    ASSERT_TRUE(moved.integer());
    ASSERT_FALSE(moved.boolean());
    ASSERT_FALSE(moved.string());
    ASSERT_FALSE(moved.object());
    ASSERT_EQ(moved.integer(), 9283847l);
}

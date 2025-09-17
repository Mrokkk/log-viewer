#include <gtest/gtest.h>

#include "utils/maybe.hpp"

using namespace utils;

TEST(MaybeTests, int)
{
    Maybe<int> m;

    ASSERT_FALSE(m);

    m = 4;

    ASSERT_TRUE(m);
    ASSERT_EQ(*m, 4);
    ASSERT_EQ(m, 4);

    m = 93847;

    ASSERT_TRUE(m);
    ASSERT_EQ(*m, 93847);
    ASSERT_EQ(m, 93847);
}

TEST(MaybeTests, intReference)
{
    Maybe<int&> m;

    ASSERT_FALSE(m);

    int value = 234;
    m = value;

    ASSERT_TRUE(m);
    ASSERT_EQ(*m, value);
    ASSERT_EQ(*m, 234);

    value = 982;
    ASSERT_TRUE(m);
    ASSERT_EQ(*m, value);
    ASSERT_EQ(*m, 982);
}

#include <gtest/gtest.h>

#include "utils/bitflag.hpp"

using namespace utils;

DEFINE_BITFLAG(TestBitFlag, int,
{
    a, b, c, d, e
});

constexpr int a = 1 << 0;
constexpr int b = 1 << 1;
constexpr int c = 1 << 2;
constexpr int d = 1 << 3;
constexpr int e = 1 << 4;

TEST(BitFlagTests, canDoStuff)
{
    TestBitFlag flag1;

    ASSERT_EQ(flag1.value, 0);

    flag1 |= TestBitFlag::a;
    ASSERT_EQ(flag1.value, 1);
    ASSERT_EQ(flag1 & TestBitFlag::b, 0);

    flag1 |= TestBitFlag::b;
    ASSERT_EQ((flag1 & TestBitFlag::a).value, a);
    ASSERT_EQ((flag1 & TestBitFlag::b).value, b);
    ASSERT_EQ(flag1.value, a | b);

    flag1 &= TestBitFlag::c;
    ASSERT_EQ((flag1 & TestBitFlag::a).value, 0);
    ASSERT_EQ((flag1 & TestBitFlag::b).value, 0);
    ASSERT_EQ((flag1 & TestBitFlag::c).value, 0);
    ASSERT_EQ(flag1.value, 0);

    flag1 |= TestBitFlag::a | TestBitFlag::b | TestBitFlag::d;
    ASSERT_EQ((flag1 & TestBitFlag::a).value, a);
    ASSERT_EQ((flag1 & TestBitFlag::b).value, b);
    ASSERT_EQ((flag1 & TestBitFlag::c).value, 0);
    ASSERT_EQ((flag1 & TestBitFlag::d).value, d);
    ASSERT_EQ(flag1.value, a | b | d);

    flag1 ^= TestBitFlag::c;
    ASSERT_EQ(flag1.value, a | b | c | d);

    flag1 ^= TestBitFlag::a;
    ASSERT_EQ(flag1.value, b | c | d);

    flag1 ^= TestBitFlag::e;
    ASSERT_EQ(flag1.value, b | c | d | e);

    flag1 &= ~TestBitFlag::b;
    ASSERT_EQ(flag1.value, c | d | e);

    flag1 = ~TestBitFlag::a;
    ASSERT_EQ(flag1.value, ~a);
}

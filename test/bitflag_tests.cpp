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

static_assert(bitIndex(TestBitFlag::a) == 0);
static_assert(bitIndex(TestBitFlag::b) == 1);
static_assert(bitIndex(TestBitFlag::c) == 2);
static_assert(bitIndex(TestBitFlag::d) == 3);
static_assert(bitIndex(TestBitFlag::e) == 4);

TEST(BitFlagTests, canDoStuff)
{
    TestBitFlag flag;

    ASSERT_EQ(flag.value, 0);

    flag |= TestBitFlag::a;
    ASSERT_EQ(flag.value, 1);
    ASSERT_EQ(flag[TestBitFlag::a], true);
    ASSERT_EQ(flag & TestBitFlag::b, 0);
    ASSERT_EQ(flag[TestBitFlag::b], false);

    flag |= TestBitFlag::b;
    ASSERT_EQ((flag & TestBitFlag::a).value, a);
    ASSERT_EQ((flag & TestBitFlag::b).value, b);
    ASSERT_EQ(flag.value, a | b);
    ASSERT_EQ(flag[TestBitFlag::a], true);
    ASSERT_EQ(flag[TestBitFlag::b], true);
    ASSERT_EQ(flag[TestBitFlag::c], false);

    flag &= TestBitFlag::c;
    ASSERT_EQ((flag & TestBitFlag::a).value, 0);
    ASSERT_EQ((flag & TestBitFlag::b).value, 0);
    ASSERT_EQ((flag & TestBitFlag::c).value, 0);
    ASSERT_EQ(flag.value, 0);

    flag |= TestBitFlag::a | TestBitFlag::b | TestBitFlag::d;
    ASSERT_EQ((flag & TestBitFlag::a).value, a);
    ASSERT_EQ((flag & TestBitFlag::b).value, b);
    ASSERT_EQ((flag & TestBitFlag::c).value, 0);
    ASSERT_EQ((flag & TestBitFlag::d).value, d);
    ASSERT_EQ(flag.value, a | b | d);

    flag ^= TestBitFlag::c;
    ASSERT_EQ(flag.value, a | b | c | d);

    flag ^= TestBitFlag::a;
    ASSERT_EQ(flag.value, b | c | d);

    flag ^= TestBitFlag::e;
    ASSERT_EQ(flag.value, b | c | d | e);

    flag &= ~TestBitFlag::b;
    ASSERT_EQ(flag.value, c | d | e);

    auto test = flag & (TestBitFlag::c | TestBitFlag::d);
    ASSERT_EQ(test.value, c | d);

    auto test2 = flag & ~TestBitFlag::c;
    ASSERT_EQ(test2.value, d | e);

    flag = ~TestBitFlag::a;
    ASSERT_EQ(flag.value, ~a);
}

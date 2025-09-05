#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/ring_buffer.hpp"

using namespace utils;

static const struct ToVector{} toVector;

template <typename T>
std::vector<T> operator|(RingBuffer<T>& buf, const ToVector&)
{
    std::vector<T> vec;
    buf.forEach([&](const auto& val){ vec.push_back(val); });
    return vec;
}

TEST(RingBufferTests, canPushFront)
{
    RingBuffer<int> buf(3);

    ASSERT_EQ(buf.size(), 0);
    ASSERT_EQ(buf.capacity(), 3);

    buf.pushFront(0);
    ASSERT_EQ(buf.size(), 1);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0));

    buf.pushFront(1);
    ASSERT_EQ(buf.size(), 2);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(1, 0));

    buf.pushFront(2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(2, 1, 0));

    buf.pushFront(3);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(3, 2, 1));
}

TEST(RingBufferTests, canPushBack)
{
    RingBuffer<int> buf(3);

    ASSERT_EQ(buf.size(), 0);
    ASSERT_EQ(buf.capacity(), 3);

    buf.pushBack(0);
    ASSERT_EQ(buf.size(), 1);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0));

    buf.pushBack(1);
    ASSERT_EQ(buf.size(), 2);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1));

    buf.pushBack(2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1, 2));

    buf.pushBack(3);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(1, 2, 3));
}

TEST(RingBufferTests, canBeCleared)
{
    RingBuffer<int> buf(3);

    ASSERT_EQ(buf.size(), 0);
    ASSERT_EQ(buf.capacity(), 3);

    buf.pushBack(0);
    ASSERT_EQ(buf.size(), 1);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0));

    buf.pushBack(1);
    ASSERT_EQ(buf.size(), 2);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1));

    buf.pushFront(2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(2, 0, 1));

    buf.pushBack(3);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1, 3));

    buf.clear();
    ASSERT_EQ(buf.size(), 0);
    ASSERT_EQ(buf.capacity(), 3);
    ASSERT_THAT(buf | toVector, testing::IsEmpty());
}

TEST(RingBufferTests, canPushBackAndFront)
{
    RingBuffer<int> buf(3);

    ASSERT_EQ(buf.size(), 0);
    ASSERT_EQ(buf.capacity(), 3);

    buf.pushBack(0);
    ASSERT_EQ(buf.size(), 1);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0));

    buf.pushBack(1);
    ASSERT_EQ(buf.size(), 2);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1));

    buf.pushBack(2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1, 2));

    buf.pushFront(-1);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(-1, 0, 1));

    buf.pushFront(-2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(-2, -1, 0));

    buf.pushBack(1);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(-1, 0, 1));

    buf.pushBack(2);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(0, 1, 2));

    buf.pushBack(3);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(1, 2, 3));

    buf.pushBack(4);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(2, 3, 4));

    buf.pushBack(5);
    ASSERT_EQ(buf.size(), 3);
    ASSERT_THAT(buf | toVector, testing::ElementsAre(3, 4, 5));
}

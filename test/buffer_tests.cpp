#include <cfloat>
#include <climits>

#include <gtest/gtest.h>

#include "utils/buffer.hpp"
#include "utils/format.hpp"

using namespace utils;

enum SomeEnum
{
    value1 = 0,
    value2 = 1,
};

TEST(BufferTests, newBufferIsEmpty)
{
    Buffer buf;

    ASSERT_EQ(buf.length(), 0);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity);
    ASSERT_EQ(buf.view(), "");
    ASSERT_TRUE(buf.buffer());
}

TEST(BufferTests, canPrintChar)
{
    {
        Buffer buf;
        buf << 'a';

        ASSERT_EQ(buf.view(), "a");
    }
    {
        Buffer buf;
        buf << '$';

        ASSERT_EQ(buf.view(), "$");
    }
}

TEST(BufferTests, canPrintUnsignedChar)
{
    Buffer buf;
    buf << UCHAR_MAX;

    ASSERT_EQ(buf.view(), "255");
}

TEST(BufferTests, canPrintInt)
{
    {
        Buffer buf;
        buf << INT_MAX;

        ASSERT_EQ(buf.view(), "2147483647");
    }
    {
        Buffer buf;
        buf << INT_MIN;

        ASSERT_EQ(buf.view(), "-2147483648");
    }
}

TEST(BufferTests, canPrintUnsignedInt)
{
    {
        Buffer buf;
        buf << UINT_MAX;

        ASSERT_EQ(buf.view(), "4294967295");
    }
    {
        Buffer buf;
        buf << 0u;

        ASSERT_EQ(buf.view(), "0");
    }
}

TEST(BufferTests, canPrintLong)
{
    {
        Buffer buf;
        buf << LONG_MAX;

        ASSERT_EQ(buf.view(), "9223372036854775807");
    }
    {
        Buffer buf;
        buf << LONG_MIN;

        ASSERT_EQ(buf.view(), "-9223372036854775808");
    }
}

TEST(BufferTests, canPrintUnsignedLong)
{
    {
        Buffer buf;
        buf << ULONG_MAX;

        ASSERT_EQ(buf.view(), "18446744073709551615");
    }
    {
        Buffer buf;
        buf << 0ul;

        ASSERT_EQ(buf.view(), "0");
    }
}

TEST(BufferTests, canPrintFloat)
{
    {
        Buffer buf;
        buf << FLT_MAX;

        ASSERT_EQ(buf.view(), "340282346638528859811704183484516925440.000000");
    }
    {
        Buffer buf;
        buf << FLT_MIN;

        ASSERT_EQ(buf.view(), "0.000000");
    }
}

TEST(BufferTests, canPrintBool)
{
    {
        Buffer buf;
        buf << true;

        ASSERT_EQ(buf.view(), "true");
    }
    {
        Buffer buf;
        buf << false;

        ASSERT_EQ(buf.view(), "false");
    }
}

TEST(BufferTests, canPrintEnum)
{
    {
        Buffer buf;
        buf << SomeEnum::value1;

        ASSERT_EQ(buf.view(), "0");
    }
    {
        Buffer buf;
        buf << SomeEnum::value2;

        ASSERT_EQ(buf.view(), "1");
    }
}

TEST(BufferTests, canPrintPointer)
{
    Buffer buf;

    auto ptr = reinterpret_cast<std::string*>(0xfffffffffffffffful);
    buf << ptr;

    ASSERT_EQ(buf.view(), "0xffffffffffffffff");
}

TEST(BufferTests, canPrintStringView)
{
    Buffer buf;
    buf << std::string_view("test string view");

    ASSERT_EQ(buf.view(), "test string view");
}

TEST(BufferTests, canPrintString)
{
    Buffer buf;
    buf << std::string_view("test string");

    ASSERT_EQ(buf.view(), "test string");
}

TEST(BufferTests, canPrintf)
{
    Buffer buf;
    buf.printf("Hello %u %p", 9382, (void*)0xffff)
       .printf(" %.1f", 33.3)
       .printf(" %#x", 0x324);

    ASSERT_EQ(buf.view(), "Hello 9382 0xffff 33.3 0x324");
}

TEST(BufferTests, canExtendCapacity)
{
    Buffer buf;

    buf << std::string(Buffer::initialCapacity, 'k');

    ASSERT_EQ(buf.length(), Buffer::initialCapacity);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity);

    buf << 'a';

    ASSERT_EQ(buf.length(), Buffer::initialCapacity + 1);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 2);

    buf << std::string(Buffer::initialCapacity - 2, 'k');

    ASSERT_EQ(buf.length(), Buffer::initialCapacity * 2 - 1);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 2);

    buf << 938;

    ASSERT_EQ(buf.length(), Buffer::initialCapacity * 2 + 2);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 4);
}

TEST(BufferTests, canBeCleared)
{
    Buffer buf;

    buf << "test";

    ASSERT_EQ(buf.length(), 4);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity);
    ASSERT_EQ(buf.view(), "test");

    buf.clear();

    ASSERT_EQ(buf.length(), 0);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity);
    ASSERT_EQ(buf.view(), "");
}

TEST(BufferTests, canBeMoved)
{
    {
        Buffer buf1;
        Buffer buf2;

        buf1 << "abcdef";
        buf2 << "qwerty";

        buf1 = std::move(buf2);

        ASSERT_EQ(buf1.length(), 6);
        ASSERT_EQ(buf1.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf1.view(), "qwerty");

        ASSERT_EQ(buf2.length(), 0);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf2.view(), "");

        buf2 = std::move(buf1);

        ASSERT_EQ(buf1.length(), 0);
        ASSERT_EQ(buf1.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf1.view(), "");

        ASSERT_EQ(buf2.length(), 6);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf2.view(), "qwerty");
    }
    {
        Buffer buf1;
        Buffer buf2;

        buf1 << std::string(128, 'a');
        buf2 << std::string(128, 'b');

        buf1 = std::move(buf2);

        ASSERT_EQ(buf1.length(), 128);
        ASSERT_EQ(buf1.capacity(), Buffer::initialCapacity * 2);
        ASSERT_EQ(buf1.view()[0], 'b');

        ASSERT_EQ(buf2.length(), 0);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf2.view(), "");
    }
    {
        Buffer buf1;

        buf1 << std::string(128, 'a');

        Buffer buf2(std::move(buf1));

        ASSERT_EQ(buf2.length(), 128);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity * 2);
        ASSERT_EQ(buf2.view()[0], 'a');

        ASSERT_EQ(buf1.length(), 0);
        ASSERT_EQ(buf1.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf1.view(), "");
    }
}

TEST(BufferTests, canFormat)
{
    auto buf = utils::format("Hello {} test {} test2 {}", 33, 44, std::string_view("test string view"));
    ASSERT_EQ(buf.view(), "Hello 33 test 44 test2 test string view");
}

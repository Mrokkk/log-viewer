#include <cfloat>
#include <climits>
#include <cstring>
#include <filesystem>

#include <gtest/gtest.h>

#include "utils/buffer.hpp"
#include "utils/format.hpp"

using namespace utils;

enum SomeEnum
{
    value1 = 0,
    value2 = 1,
};

#define ASSERT_BUF(BUF, STRING, CAPACITY) \
    do \
    { \
        constexpr auto len = sizeof(STRING) - 1; \
        ASSERT_EQ(BUF.length(), len); \
        ASSERT_EQ(BUF.size(), len); \
        ASSERT_EQ(BUF.capacity(), CAPACITY); \
        ASSERT_EQ(BUF.view(), STRING); \
        ASSERT_EQ(std::strlen(BUF.data()), len); \
    } \
    while (0)


TEST(BufferTests, newBufferIsEmpty)
{
    Buffer buf;

    ASSERT_BUF(buf, "", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintChar)
{
    {
        Buffer buf;
        buf << 'a';

        ASSERT_BUF(buf, "a", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << '$' << 'b';

        ASSERT_BUF(buf, "$b", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintUnsignedChar)
{
    Buffer buf;
    buf << UCHAR_MAX;

    ASSERT_BUF(buf, "255", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintInt)
{
    {
        Buffer buf;
        buf << INT_MAX;

        ASSERT_BUF(buf, "2147483647", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << INT_MIN;

        ASSERT_BUF(buf, "-2147483648", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintUnsignedInt)
{
    {
        Buffer buf;
        buf << UINT_MAX;

        ASSERT_BUF(buf, "4294967295", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << 0u;

        ASSERT_BUF(buf, "0", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintLong)
{
    {
        Buffer buf;
        buf << LONG_MAX;

        ASSERT_BUF(buf, "9223372036854775807", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << LONG_MIN;

        ASSERT_BUF(buf, "-9223372036854775808", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintUnsignedLong)
{
    {
        Buffer buf;
        buf << ULONG_MAX;

        ASSERT_BUF(buf, "18446744073709551615", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << 0ul;

        ASSERT_BUF(buf, "0", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintFloat)
{
    {
        Buffer buf;
        buf << FLT_MAX;

        ASSERT_BUF(buf, "340282346638528859811704183484516925440.000000", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << FLT_MIN;

        ASSERT_BUF(buf, "0.000000", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintBool)
{
    {
        Buffer buf;
        buf << true;

        ASSERT_BUF(buf, "true", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << false;

        ASSERT_BUF(buf, "false", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintEnum)
{
    {
        Buffer buf;
        buf << SomeEnum::value1;

        ASSERT_BUF(buf, "0", Buffer::initialCapacity);
    }
    {
        Buffer buf;
        buf << SomeEnum::value2;

        ASSERT_BUF(buf, "1", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintPointer)
{
    {
        Buffer buf;

        auto ptr = reinterpret_cast<std::string*>(0xfffffffffffffffful);
        buf << ptr;

        ASSERT_BUF(buf, "0xffffffffffffffff", Buffer::initialCapacity);
    }
    {
        Buffer buf;

        auto ptr = reinterpret_cast<const std::string*>(0x1234ul);
        buf << ptr;

        ASSERT_BUF(buf, "0x1234", Buffer::initialCapacity);
    }
}

TEST(BufferTests, canPrintCStr)
{
    Buffer buf;
    buf << "test cstr";

    ASSERT_BUF(buf, "test cstr", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintStringView)
{
    Buffer buf;
    buf << std::string_view("test string view");

    ASSERT_BUF(buf, "test string view", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintString)
{
    Buffer buf;
    buf << std::string("test string");

    ASSERT_BUF(buf, "test string", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintPath)
{
    Buffer buf;
    std::filesystem::path path("/usr/bin/bash");
    buf << path;

    ASSERT_BUF(buf, "/usr/bin/bash", Buffer::initialCapacity);
}

TEST(BufferTests, canExtendCapacity)
{
    Buffer buf;

    buf << std::string(Buffer::initialCapacity - 1, 'k');

    ASSERT_EQ(buf.length(), Buffer::initialCapacity - 1);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity);

    buf << 'a';

    ASSERT_EQ(buf.length(), Buffer::initialCapacity);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 2);

    buf << std::string(Buffer::initialCapacity - 2, 'k');

    ASSERT_EQ(buf.length(), Buffer::initialCapacity * 2 - 2);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 2);

    buf << 938;

    ASSERT_EQ(buf.length(), Buffer::initialCapacity * 2 + 1);
    ASSERT_EQ(buf.capacity(), Buffer::initialCapacity * 4);
}

TEST(BufferTests, canBeCleared)
{
    Buffer buf;

    buf << "test";

    ASSERT_BUF(buf, "test", Buffer::initialCapacity);

    buf.clear();

    ASSERT_BUF(buf, "", Buffer::initialCapacity);
}

TEST(BufferTests, canBeMoved)
{
    {
        Buffer buf1;
        Buffer buf2;

        buf1 << "abcdef";
        buf2 << "qwerty";

        buf1 = std::move(buf2);

        ASSERT_BUF(buf1, "qwerty", Buffer::initialCapacity);
        ASSERT_BUF(buf2, "", Buffer::initialCapacity);

        buf2 = std::move(buf1);

        ASSERT_BUF(buf1, "", Buffer::initialCapacity);
        ASSERT_BUF(buf2, "qwerty", Buffer::initialCapacity);
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
        ASSERT_EQ(std::strlen(buf1.data()), 128);

        ASSERT_EQ(buf2.length(), 0);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf2.view(), "");
        ASSERT_EQ(std::strlen(buf2.data()), 0);
    }
    {
        Buffer buf1;

        buf1 << std::string(128, 'a');

        Buffer buf2(std::move(buf1));

        ASSERT_EQ(buf2.length(), 128);
        ASSERT_EQ(buf2.capacity(), Buffer::initialCapacity * 2);
        ASSERT_EQ(buf2.view()[0], 'a');
        ASSERT_EQ(std::strlen(buf2.data()), 128);

        ASSERT_EQ(buf1.length(), 0);
        ASSERT_EQ(buf1.capacity(), Buffer::initialCapacity);
        ASSERT_EQ(buf1.view(), "");
        ASSERT_EQ(std::strlen(buf1.data()), 0);
    }
}

TEST(BufferTests, canBeConvertedToStringView)
{
    Buffer buf;
    buf << 123;
    std::string_view result = buf;
    ASSERT_EQ(result, "123");
}

TEST(BufferTests, canCreateString)
{
    Buffer buf;
    buf << 9876;
    auto result = buf.str();
    ASSERT_EQ(result, "9876");
}

TEST(BufferTests, canIterate)
{
    std::string_view testString("test string");
    Buffer buf;
    buf << testString;
    int i = 0;
    for (auto c : buf)
    {
        ASSERT_EQ(c, testString[i++]);
    }
}

TEST(BufferTests, canPrintHexInt)
{
    Buffer buf;
    buf << (INT_MAX | hex(showbase));
    ASSERT_BUF(buf, "0x7fffffff", Buffer::initialCapacity);

    buf.clear();
    buf << (INT_MIN | hex(noshowbase));
    ASSERT_BUF(buf, "80000000", Buffer::initialCapacity);

    buf.clear();
    buf << (int(0) | hex(showbase));
    ASSERT_BUF(buf, "0", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintOctInt)
{
    Buffer buf;
    buf << (INT_MAX | oct(showbase));
    ASSERT_BUF(buf, "017777777777", Buffer::initialCapacity);

    buf.clear();
    buf << (INT_MIN | oct(noshowbase));
    ASSERT_BUF(buf, "20000000000", Buffer::initialCapacity);

    buf.clear();
    buf << (int(0) | oct(showbase));
    ASSERT_BUF(buf, "0", Buffer::initialCapacity);
}

TEST(BufferTests, canPrintFloatWithGivenPrecision)
{
    Buffer buf;
    buf << (0.33f | precision(2));
    ASSERT_BUF(buf, "0.33", Buffer::initialCapacity);

    buf.clear();
    buf << (0.12345f | precision(2));
    ASSERT_BUF(buf, "0.12", Buffer::initialCapacity);

    buf.clear();
    buf << (0.88f | precision(4));
    ASSERT_BUF(buf, "0.8800", Buffer::initialCapacity);

    buf.clear();
    buf << (0.12f | precision(0));
    ASSERT_BUF(buf, "0", Buffer::initialCapacity);

    buf.clear();
    buf << (0.55f | precision(0));
    ASSERT_BUF(buf, "1", Buffer::initialCapacity);

    buf.clear();
    buf << (0.55f | precision(21));
    ASSERT_EQ(buf.length(), 23);
}

TEST(BufferTests, hexAndOctOveridesEachOther)
{
    Buffer buf;
    buf << (INT_MAX | oct(showbase) | hex(noshowbase));
    ASSERT_BUF(buf, "7fffffff", Buffer::initialCapacity);

    buf.clear();
    buf << (INT_MIN | hex(noshowbase) | oct(showbase));
    ASSERT_BUF(buf, "020000000000", Buffer::initialCapacity);
}

TEST(BufferTests, canDoPadding)
{
    Buffer buf;
    buf << (0xfff | hex(showbase) | leftPadding(10));
    ASSERT_BUF(buf, "     0xfff", Buffer::initialCapacity);

    buf.clear();
    buf << (0x974 | rightPadding(6) | hex(showbase));
    ASSERT_BUF(buf, "0x974 ", Buffer::initialCapacity);

    buf.clear();
    buf << ('c' | rightPadding(6));
    ASSERT_BUF(buf, "c     ", Buffer::initialCapacity);

    buf.clear();
    buf << (std::string_view("test") | leftPadding(6)) << ("test2" | rightPadding(7));
    ASSERT_BUF(buf, "  testtest2  ", Buffer::initialCapacity);

    buf.clear();
    buf << (std::string("test") | leftPadding(6));
    ASSERT_BUF(buf, "  test", Buffer::initialCapacity);

    buf.clear();
    buf << ((void*)0xfff | leftPadding(10));
    ASSERT_BUF(buf, "     0xfff", Buffer::initialCapacity);

    buf.clear();
    buf << (0.32f | precision(1) | leftPadding(10));
    ASSERT_BUF(buf, "       0.3", Buffer::initialCapacity);

    buf.clear();
    buf << (0.3272f | rightPadding(5) | precision(2));
    ASSERT_BUF(buf, "0.33 ", Buffer::initialCapacity);

    buf.clear();
    buf << (std::filesystem::path("/usr/bin") | leftPadding(10));
    ASSERT_BUF(buf, "  /usr/bin", Buffer::initialCapacity);

    buf.clear();
    buf << (SomeEnum::value1 | leftPadding(10));
    ASSERT_BUF(buf, "         0", Buffer::initialCapacity);
}

TEST(BufferTests, canWrite)
{
    Buffer buf;
    std::string_view testString("test string");

    buf.write(testString.data(), testString.size());
    ASSERT_BUF(buf, "test string", Buffer::initialCapacity);
}

TEST(BufferTests, canFormat)
{
    auto buf = utils::format("Hello {} test {} test2 {}", 33, 44, std::string_view("test string view"));
    ASSERT_BUF(buf, "Hello 33 test 44 test2 test string view", Buffer::initialCapacity);
}

struct TestStruct
{
    int a;
};

Buffer& operator<<(Buffer& buf, const TestStruct& s)
{
    return buf << "{a: " << s.a << '}';
}

TEST(BufferTests, canPrintCustomType)
{
    Buffer buf;
    TestStruct s{29485};
    buf << (s | leftPadding(12));
    ASSERT_BUF(buf, "  {a: 29485}", Buffer::initialCapacity);
}

#include <cfloat>
#include <climits>

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

TEST(BufferTests, newBufferIsEmpty)
{
    Buffer buf;

    ASSERT_EQ(buf.length(), 0);
    ASSERT_EQ(buf.size(), 0);
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
        ASSERT_EQ(buf.size(), 1);
    }
    {
        Buffer buf;
        buf << '$' << 'b';

        ASSERT_EQ(buf.view(), "$b");
        ASSERT_EQ(buf.size(), 2);
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
    {
        Buffer buf;

        auto ptr = reinterpret_cast<std::string*>(0xfffffffffffffffful);
        buf << ptr;

        ASSERT_EQ(buf.view(), "0xffffffffffffffff");
    }
    {
        Buffer buf;

        auto ptr = reinterpret_cast<const std::string*>(0x1234ul);
        buf << ptr;

        ASSERT_EQ(buf.view(), "0x1234");
    }
}

TEST(BufferTests, canPrintCStr)
{
    Buffer buf;
    buf << "test cstr";

    ASSERT_EQ(buf.view(), "test cstr");
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
    buf << std::string("test string");

    ASSERT_EQ(buf.view(), "test string");
}

TEST(BufferTests, canPrintPath)
{
    Buffer buf;
    std::filesystem::path path("/usr/bin/bash");
    buf << path;

    ASSERT_EQ(buf.view(), "/usr/bin/bash");
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
    ASSERT_EQ(buf.view(), "0x7fffffff");

    buf.clear();
    buf << (INT_MIN | hex(noshowbase));
    ASSERT_EQ(buf.view(), "80000000");

    buf.clear();
    buf << (int(0) | hex(showbase));
    ASSERT_EQ(buf.view(), "0");
}

TEST(BufferTests, canPrintOctInt)
{
    Buffer buf;
    buf << (INT_MAX | oct(showbase));
    ASSERT_EQ(buf.view(), "017777777777");

    buf.clear();
    buf << (INT_MIN | oct(noshowbase));
    ASSERT_EQ(buf.view(), "20000000000");

    buf.clear();
    buf << (int(0) | oct(showbase));
    ASSERT_EQ(buf.view(), "0");
}

TEST(BufferTests, canPrintFloatWithGivenPrecision)
{
    Buffer buf;
    buf << (0.33f | precision(2));
    ASSERT_EQ(buf.view(), "0.33");

    buf.clear();
    buf << (0.12345f | precision(2));
    ASSERT_EQ(buf.view(), "0.12");

    buf.clear();
    buf << (0.88f | precision(4));
    ASSERT_EQ(buf.view(), "0.8800");

    buf.clear();
    buf << (0.12f | precision(0));
    ASSERT_EQ(buf.view(), "0");

    buf.clear();
    buf << (0.55f | precision(0));
    ASSERT_EQ(buf.view(), "1");

    buf.clear();
    buf << (0.55f | precision(21));
    ASSERT_EQ(buf.length(), 23);
}

TEST(BufferTests, hexAndOctOveridesEachOther)
{
    Buffer buf;
    buf << (INT_MAX | oct(showbase) | hex(noshowbase));
    ASSERT_EQ(buf.view(), "7fffffff");

    buf.clear();
    buf << (INT_MIN | hex(noshowbase) | oct(showbase));
    ASSERT_EQ(buf.view(), "020000000000");
}

TEST(BufferTests, canDoPadding)
{
    Buffer buf;
    buf << (0xfff | hex(showbase) | leftPadding(10));
    ASSERT_EQ(buf.view(), "     0xfff");

    buf.clear();
    buf << (0x974 | rightPadding(6) | hex(showbase));
    ASSERT_EQ(buf.view(), "0x974 ");

    buf.clear();
    buf << ('c' | rightPadding(6));
    ASSERT_EQ(buf.view(), "c     ");

    buf.clear();
    buf << (std::string_view("test") | leftPadding(6)) << ("test2" | rightPadding(7));
    ASSERT_EQ(buf.view(), "  testtest2  ");

    buf.clear();
    buf << (std::string("test") | leftPadding(6));
    ASSERT_EQ(buf.view(), "  test");

    buf.clear();
    buf << ((void*)0xfff | leftPadding(10));
    ASSERT_EQ(buf.view(), "     0xfff");

    buf.clear();
    buf << (0.32f | precision(1) | leftPadding(10));
    ASSERT_EQ(buf.view(), "       0.3");

    buf.clear();
    buf << (0.3272f | rightPadding(5) | precision(2));
    ASSERT_EQ(buf.view(), "0.33 ");

    buf.clear();
    buf << (std::filesystem::path("/usr/bin") | leftPadding(10));
    ASSERT_EQ(buf.view(), "  /usr/bin");

    buf.clear();
    buf << (SomeEnum::value1 | leftPadding(10));
    ASSERT_EQ(buf.view(), "         0");
}

TEST(BufferTests, canWrite)
{
    Buffer buf;
    std::string_view testString("test string");

    buf.write(testString.data(), testString.size());
    ASSERT_EQ(buf.view(), "test string");
}

TEST(BufferTests, canFormat)
{
    auto buf = utils::format("Hello {} test {} test2 {}", 33, 44, std::string_view("test string view"));
    ASSERT_EQ(buf.view(), "Hello 33 test 44 test2 test string view");
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
    ASSERT_EQ(buf.view(), "  {a: 29485}");
}

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/hash_map.hpp"

using namespace utils;

TEST(HashMapTests, isDefaultConstructibleAsEmpty)
{
    HashMap<std::string, int> map;

    ASSERT_EQ(map.size(), 0);
    ASSERT_FALSE(map.find("aaa"));
    ASSERT_FALSE(map.find("bbb"));
}

TEST(HashMapTests, isConstructibleWithInitializerList)
{
    HashMap<std::string, int> map{
        {"test", 1},
        {"string", 2}
    };

    ASSERT_EQ(map.size(), 2);

    auto node = map.find("test");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test");
    ASSERT_EQ(node->second, 1);

    node = map.find("string");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "string");
    ASSERT_EQ(node->second, 2);

    ASSERT_FALSE(map.find("test1"));
    ASSERT_FALSE(map.find("string2"));
}

TEST(HashMapTests, isMovable)
{
    HashMap<std::string, int> map{
        {"test", 1},
        {"string", 2}
    };

    ASSERT_EQ(map.size(), 2);

    HashMap<std::string, int> map2(std::move(map));

    ASSERT_EQ(map.size(), 0);
    ASSERT_FALSE(map.find("test"));
    ASSERT_FALSE(map.find("string"));

    ASSERT_EQ(map2.size(), 2);
    ASSERT_TRUE(map2.find("test"));
    ASSERT_TRUE(map2.find("string"));

    map = std::move(map2);

    ASSERT_EQ(map2.size(), 0);
    ASSERT_FALSE(map2.find("test"));
    ASSERT_FALSE(map2.find("string"));

    ASSERT_EQ(map.size(), 2);
    ASSERT_TRUE(map.find("test"));
    ASSERT_TRUE(map.find("string"));
}

TEST(HashMapTests, canInsertElements)
{
    HashMap<std::string, int> map;
    for (int i = 0; i < 1024; ++i)
    {
        map.insert(std::to_string(i), i);
    }

    ASSERT_EQ(map.size(), 1024);

    for (int i = 0; i < 1024; ++i)
    {
        auto node = map.find(std::to_string(i));
        ASSERT_TRUE(node);
        ASSERT_EQ(node->second, i);
    }

    ASSERT_FALSE(map.find("24872"));
    ASSERT_FALSE(map.find("-193"));
}

TEST(HashMapTests, canReplaceElements)
{
    HashMap<std::string, int> map{
        {"test", 1},
        {"string", 2}
    };

    ASSERT_EQ(map.size(), 2);

    map.insert("test", 9283);

    ASSERT_EQ(map.size(), 2);

    auto node = map.find("test");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test");
    ASSERT_EQ(node->second, 9283);
}

TEST(HashMapTests, canEraseElements)
{
    HashMap<std::string, int> map{
        {"test", 1},
        {"string", 2},
        {"string2", 3}
    };

    map.erase("aaa");
    map.erase("bbb");
    map.erase("ccc");

    ASSERT_EQ(map.size(), 3);
    ASSERT_TRUE(map.find("test"));
    ASSERT_TRUE(map.find("string"));
    ASSERT_TRUE(map.find("string2"));

    map.erase("test");

    ASSERT_EQ(map.size(), 2);
    ASSERT_FALSE(map.find("test"));
    ASSERT_TRUE(map.find("string"));
    ASSERT_TRUE(map.find("string2"));

    map.erase("string");

    ASSERT_EQ(map.size(), 1);
    ASSERT_FALSE(map.find("test"));
    ASSERT_FALSE(map.find("string"));
    ASSERT_TRUE(map.find("string2"));

    map.erase("string2");

    ASSERT_EQ(map.size(), 0);
    ASSERT_FALSE(map.find("test"));
    ASSERT_FALSE(map.find("string"));
    ASSERT_FALSE(map.find("string2"));
}

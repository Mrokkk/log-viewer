#include <string_view>

#include <gtest/gtest.h>

#include "utils/trie.hpp"

using namespace utils;

TEST(TrieTests, isConstrictibleWithInitializerList)
{
    Trie<int> trie{
        {"test1", 0},
        {"test2", 1},
    };

    ASSERT_FALSE(trie.find("test"));
    ASSERT_FALSE(trie.find("test3"));

    auto node = trie.find("test1");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test1");
    ASSERT_EQ(node->second, 0);

    node = trie.find("test2");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test2");
    ASSERT_EQ(node->second, 1);
}

TEST(TrieTests, canAddAndFindElements)
{
    Trie<int> trie;

    trie.insert("test1", 0);
    trie.insert("test2", 1);

    ASSERT_FALSE(trie.find("test"));
    ASSERT_FALSE(trie.find("test3"));

    auto node = trie.find("test1");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test1");
    ASSERT_EQ(node->second, 0);

    node = trie.find("test2");

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "test2");
    ASSERT_EQ(node->second, 1);
}

TEST(TrieTests, canEraseElements)
{
    Trie<int> trie;

    trie.insert("test1", 0);
    trie.insert("test2", 1);
    trie.insert("string", 2);

    ASSERT_TRUE(trie.find("test1"));
    ASSERT_TRUE(trie.find("test2"));
    ASSERT_TRUE(trie.find("string"));

    trie.erase("test1");

    ASSERT_FALSE(trie.find("test1"));
    ASSERT_TRUE(trie.find("test2"));
    ASSERT_TRUE(trie.find("string"));

    trie.erase("test2");

    ASSERT_FALSE(trie.find("test1"));
    ASSERT_FALSE(trie.find("test2"));
    ASSERT_TRUE(trie.find("string"));

    trie.erase("string");

    ASSERT_FALSE(trie.find("test1"));
    ASSERT_FALSE(trie.find("test2"));
    ASSERT_FALSE(trie.find("s"));
    ASSERT_FALSE(trie.find("st"));
    ASSERT_FALSE(trie.find("str"));
    ASSERT_FALSE(trie.find("string"));
}

TEST(TrieTests, canScan_1)
{
    Trie<int> trie;

    trie.insert("aaa", 0);
    trie.insert("bbb", 3);

    ASSERT_FALSE(trie.find("a"));
    ASSERT_FALSE(trie.find("aa"));
    ASSERT_TRUE(trie.find("aaa"));
    ASSERT_FALSE(trie.find("aaaa"));
    ASSERT_FALSE(trie.find("b"));
    ASSERT_FALSE(trie.find("bb"));
    ASSERT_TRUE(trie.find("bbb"));
    ASSERT_FALSE(trie.find("bbbb"));

    std::string_view sv("abc aaa kkk bbb");

    auto ctx = trie.createScanContext();

    auto node = trie.scan(sv, ctx);

    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 7);
    ASSERT_EQ(node->first, "aaa");
    ASSERT_EQ(node->second, 0);

    node = trie.scan(sv, ctx);

    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "bbb");
    ASSERT_EQ(node->second, 3);

    node = trie.scan(sv, ctx);
    ASSERT_FALSE(node);

    ASSERT_TRUE(trie.erase("aaa"));

    ctx.reset();
    node = trie.scan(sv, ctx);
    ASSERT_EQ(ctx.currentOffset, sv.size());
    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "bbb");
    ASSERT_EQ(node->second, 3);

    ASSERT_FALSE(trie.erase("ccc"));

    trie.insert("abc", 99);
    trie.insert("aaa", 22);

    ctx.reset();
    node = trie.scan(sv, ctx);
    ASSERT_EQ(ctx.currentOffset, 3);
    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "abc");
    ASSERT_EQ(node->second, 99);

    node = trie.scan(sv, ctx);
    ASSERT_EQ(ctx.currentOffset, 7);
    ASSERT_TRUE(node);
    ASSERT_EQ(node->first, "aaa");
    ASSERT_EQ(node->second, 22);
}

TEST(TrieTests, canScan_2)
{
    Trie<int> trie;

    trie.insert("a", 0);
    trie.insert("ab", 1);
    trie.insert("abc", 2);

    std::string_view sv("abc aaa bbb ccc");

    auto ctx = trie.createScanContext();

    auto node = trie.scan(sv, ctx);

    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 1);
    ASSERT_EQ(node->first, "a");
    ASSERT_EQ(node->second, 0);

    node = trie.scan(sv, ctx);
    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 2);
    ASSERT_EQ(node->first, "ab");
    ASSERT_EQ(node->second, 1);

    node = trie.scan(sv, ctx);
    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 3);
    ASSERT_EQ(node->first, "abc");
    ASSERT_EQ(node->second, 2);

    node = trie.scan(sv, ctx);
    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 5);
    ASSERT_EQ(node->first, "a");
    ASSERT_EQ(node->second, 0);

    node = trie.scan(sv, ctx);
    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 6);
    ASSERT_EQ(node->first, "a");
    ASSERT_EQ(node->second, 0);

    node = trie.scan(sv, ctx);
    ASSERT_TRUE(node);
    ASSERT_EQ(ctx.currentOffset, 7);
    ASSERT_EQ(node->first, "a");
    ASSERT_EQ(node->second, 0);
}

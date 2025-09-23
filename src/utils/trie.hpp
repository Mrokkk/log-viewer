#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "utils/hash_map.hpp"
#include "utils/maybe.hpp"

namespace utils
{

template <typename Value>
struct Trie
{
    using Key = std::string;
    using Index = size_t;

    constexpr static inline Index emptyIndex = SIZE_MAX;

    struct Node
    {
        constexpr Node()
            : index(emptyIndex)
        {
        }

        constexpr Node(Node&& other)
            : index(other.index)
            , children(std::move(other.children))
        {
            other.index = emptyIndex;
        }

        constexpr Node& operator=(Node&& other)
        {
            index = other.index;
            children = std::move(other.children);
            other.index = emptyIndex;
            return *this;
        }

        Index               index;
        HashMap<char, Node> children;
    };

    using NodeData = std::pair<std::string, Value>;

    constexpr Trie() = default;

    constexpr Trie(std::initializer_list<NodeData> list)
    {
        for (auto& kv : list)
        {
            insert(std::move(kv.first), std::move(kv.second));
        }
    }

    void insert(std::string key, Value value)
    {
        auto node = &mRoot;

        size_t i = 0;

        for (; i < key.size(); ++i)
        {
            auto nextNode = node->children.find(key[i]);
            if (not nextNode)
            {
                break;
            }
            node = &nextNode->second;
        }

        size_t newIndex;

        if (mFreeIndexes.empty())
        {
            newIndex = mNodesData.size();
        }
        else
        {
            newIndex = mFreeIndexes.back();
            mFreeIndexes.pop_back();
        }

        for (; i < key.size(); ++i)
        {
            auto& newNode = node->children.insert(key[i], Node());
            node = &newNode.second;
            if (i == key.size() - 1)
            {
                newNode.second.index = newIndex;
            }
        }

        mNodesData.emplace(mNodesData.begin() + newIndex, std::make_pair(std::move(key), std::move(value)));
    }

    bool erase(const std::string_view& key)
    {
        if (key.empty()) [[unlikely]]
        {
            return false;
        }

        auto node = &mRoot;
        Node* nodeToRemoveFrom = nullptr;
        char keyToRemove = 0;

        for (size_t i = 0; i < key.size(); ++i)
        {
            if (node->children.size() > 1)
            {
                keyToRemove = key[i];
                nodeToRemoveFrom = node;
            }
            else if (node->children.size() == 1)
            {
                if (not nodeToRemoveFrom)
                {
                    keyToRemove = key[i];
                    nodeToRemoveFrom = node;
                }
            }
            else
            {
                nodeToRemoveFrom = nullptr;
            }

            auto nextNode = node->children.find(key[i]);
            if (not nextNode)
            {
                return false;
            }
            node = &nextNode->second;
        }

        mFreeIndexes.emplace_back(node->index);
        mNodesData[node->index].reset();
        nodeToRemoveFrom->children.erase(keyToRemove);

        return true;
    }

    struct ScanContext
    {
        constexpr void reset()
        {
            pendingNode = nullptr;
            currentOffset = 0;
        }

        const Node* pendingNode = nullptr;
        size_t currentOffset = 0;
    };

    constexpr static ScanContext createScanContext()
    {
        return ScanContext();
    }

    const NodeData* find(const std::string_view& sv) const
    {
        auto node = &mRoot;

        for (size_t i = 0; i < sv.size();)
        {
            auto nextNode = node->children.find(sv[i]);

            if (not nextNode)
            {
                return nullptr;
            }

            node = &nextNode->second;

            ++i;

            if (node->index != emptyIndex and i == sv.size())
            {
                return &*mNodesData[node->index];
            }
        }

        return nullptr;
    }

    const NodeData* scan(const std::string_view& sv, ScanContext& context) const
    {
        auto node = context.pendingNode
            ? context.pendingNode
            : &mRoot;

        for (; context.currentOffset < sv.size();)
        {
            auto nextNode = node->children.find(sv[context.currentOffset]);

            if (not nextNode)
            {
                if (node != &mRoot)
                {
                    node = &mRoot;
                }
                else
                {
                    ++context.currentOffset;
                }
                continue;
            }

            node = &nextNode->second;

            if (node->index != emptyIndex)
            {
                ++context.currentOffset;
                if (node->children.size())
                {
                    context.pendingNode = node;
                }
                return &*mNodesData[node->index];
            }

            ++context.currentOffset;
        }

        return nullptr;
    }

private:
    Node                         mRoot;
    std::vector<Maybe<NodeData>> mNodesData;
    std::vector<Index>           mFreeIndexes;
};

}  // namespace utils

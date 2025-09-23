#pragma once

#include <initializer_list>
#include <utility>
#include <vector>

#include "utils/function_ref.hpp"
#include "utils/unique_ptr.hpp"

namespace utils
{

template <typename Key, typename Value, size_t MaxBucketCount = 32>
struct HashMap
{
    using Node = std::pair<Key, Value>;

    constexpr static inline size_t initialBucketCapacity = 4;
    constexpr static inline size_t maxBucketCount{MaxBucketCount};
    constexpr static inline std::hash<Key> hashFn{};

    struct Bucket
    {
        constexpr Bucket(Key&& key, Value&& value)
        {
            mNodes.reserve(initialBucketCapacity);
            mNodes.emplace_back(std::move(key), std::move(value));
        }

        constexpr Node* find(const Key& key)
        {
            for (auto& node : mNodes)
            {
                if (node.first == key)
                {
                    return &node;
                }
            }
            return nullptr;
        }

        constexpr Node& add(Key&& key, Value value, bool& added)
        {
            for (auto& node : mNodes)
            {
                if (node.first == key)
                {
                    node.second = std::move(value);
                    added = false;
                    return node;
                }
            }
            added = true;
            return mNodes.emplace_back(std::move(key), std::move(value));
        }

        constexpr bool erase(const Key& key)
        {
            for (auto it = mNodes.begin(); it != mNodes.end(); ++it)
            {
                if (it->first == key)
                {
                    mNodes.erase(it);
                    return true;
                }
            }
            return false;
        }

    private:
        friend HashMap;
        std::vector<Node> mNodes;
    };

    constexpr HashMap()
        : mSize(0)
        , mBuckets(nullptr)
    {
    }

    constexpr HashMap(std::initializer_list<Node>&& nodes)
        : mSize(0)
        , mBuckets(new UniquePtr<Bucket>[maxBucketCount]{})
    {
        for (auto& node : nodes)
        {
            insert(std::move(node.first), std::move(node.second));
        }
    }

    constexpr HashMap(HashMap&& other)
        : mSize(other.mSize)
        , mBuckets(other.mBuckets)
    {
        other.mBuckets = nullptr;
        other.mSize = 0;
    }

    constexpr HashMap& operator=(HashMap&& other)
    {
        mSize = other.mSize;
        mBuckets = other.mBuckets;
        other.mBuckets = nullptr;
        other.mSize = 0;
        return *this;
    }

    constexpr ~HashMap()
    {
        if (mBuckets)
        {
            delete [] mBuckets;
        }
    }

    constexpr Node& insert(Key key, Value value)
    {
        const auto hash = hashFn(key);
        const auto bucketId = hash % maxBucketCount;

        if (not mBuckets)
        {
            mBuckets = new UniquePtr<Bucket>[maxBucketCount]{};
        }

        if (not mBuckets[bucketId])
        {
            ++mSize;
            mBuckets[bucketId] = utils::makeUnique<Bucket>(std::move(key), std::move(value));
            return mBuckets[bucketId]->mNodes.back();
        }
        else
        {
            bool added{false};
            auto& ref = mBuckets[bucketId]->add(std::move(key), std::move(value), added);
            mSize += static_cast<int>(added);
            return ref;
        }
    }

    Node* find(const Key& key) const
    {
        if (not mBuckets)
        {
            return nullptr;
        }
        auto hash = hashFn(key);
        auto bucketId = hash % maxBucketCount;
        if (not mBuckets[bucketId]) [[unlikely]]
        {
            return nullptr;
        }
        return mBuckets[bucketId]->find(key);
    }

    void erase(const Key& key)
    {
        if (not mBuckets)
        {
            return;
        }
        auto hash = hashFn(key);
        auto bucketId = hash % maxBucketCount;
        if (not mBuckets[bucketId])
        {
            return;
        }
        if (mBuckets[bucketId]->erase(key))
        {
            --mSize;
        }
    }

    void forEach(const FunctionRef<void(const Node&)>& visitor) const
    {
        if (not mBuckets)
        {
            return;
        }
        for (size_t i = 0; i < maxBucketCount; ++i)
        {
            if (mBuckets[i])
            {
                for (auto& node : mBuckets[i]->mNodes)
                {
                    visitor(node);
                }
            }
        }
    }

    constexpr size_t size() const
    {
        return mSize;
    }

private:
    size_t             mSize;
    UniquePtr<Bucket>* mBuckets;
};

}  // namespace utils

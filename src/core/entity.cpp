#include "entity.hpp"

#include <bit>
#include <cstdint>
#include <list>
#include <mutex>

#include "core/buffer.hpp"
#include "utils/memory.hpp"
#include "utils/units.hpp"

namespace core
{

template <typename T>
struct alignas(64) EntityNode
{
    EntityId<T> id;
    bool        initialized;
    T           object;
};

template <typename T>
static bool operator==(const EntityId<T> lhs, const EntityId<T> rhs)
{
    static_assert(sizeof(EntityId<T>) == sizeof(uint32_t) and alignof(EntityId<T>) == alignof(uint32_t),
        "internal error: size or alignment of EntityId and uint32_t do not match");
    return *std::bit_cast<const uint32_t* const>(&lhs) == *std::bit_cast<const uint32_t* const>(&rhs);
}

constexpr static size_t SLAB_SIZE = 4_KiB;

template <typename T>
struct EntitySlab
{
    static_assert(sizeof(EntityNode<T>) < SLAB_SIZE,
        "internal error: slab size is lower than EntityNode<T>");

    static_assert(sizeof(EntityNode<T>) % 64 == 0,
        "internal error: EntityNode's size should be aligned to 64 bytes");

    constexpr static size_t count = SLAB_SIZE / sizeof(EntityNode<T>);

    constexpr EntitySlab(uint32_t index)
    {
        uint32_t i = 0;
        for (auto& node : array)
        {
            node.id = {.index = index + i, .generation = 0};
            ++i;
        }
    }

    EntityNode<T> array[count];
};

template <typename T>
struct Entities<T>::Impl final
{
    EntityWithId allocate()
    {
        std::scoped_lock lock(mLock);

        if (mFreeEntities.empty()) [[unlikely]]
        {
            auto newSlabId = mSlabs.size();
            auto& slab = mSlabs.emplace_back(newSlabId * EntitySlab<T>::count);
            mSlabsArray.emplace_back(&slab);
            for (auto& node : slab.array)
            {
                mFreeEntities.push_back(node.id);
            }
        }

        auto id = mFreeEntities.front();
        mFreeEntities.pop_front();

        auto& node = getNode(id);
        node.initialized = true;

        return {node.object, id};
    }

    struct NodeAndSlab
    {
        EntityNode<T>* node;
        EntitySlab<T>* slab;
    };

    void free(EntityId<T> id)
    {
        std::scoped_lock lock(mLock);

        auto nodeAndSlab = getNodeAndSlab(id);

        if (not nodeAndSlab.node) [[unlikely]]
        {
            return;
        }

        auto& node = *nodeAndSlab.node;

        utils::destroyAt(&node.object);
        utils::constructAt(&node.object);
        node.initialized = false;
        mFreeEntities.push_back(node.id);
    }

    T* at(EntityId<T> id)
    {
        std::scoped_lock lock(mLock);

        auto nodeAndSlab = getNodeAndSlab(id);

        if (not nodeAndSlab.node) [[unlikely]]
        {
            return nullptr;
        }

        return &nodeAndSlab.node->object;
    }

private:
    EntityNode<T>& getNode(EntityId<T> id)
    {
        auto slabId = id.index / EntitySlab<T>::count;
        auto entityId = id.index % EntitySlab<T>::count;

        auto& slab = *mSlabsArray.at(slabId);
        auto& node = slab.array[entityId];

        return node;
    }

    constexpr static EntityNode<T>* maybeInitialized(EntityNode<T>& node, bool initialized)
    {
        return std::bit_cast<EntityNode<T>*>(std::bit_cast<uintptr_t>(&node) * initialized);
    }

    NodeAndSlab getNodeAndSlab(EntityId<T> id)
    {
        auto slabId = id.index / EntitySlab<T>::count;
        auto entityId = id.index % EntitySlab<T>::count;

        if (slabId >= mSlabsArray.size()) [[unlikely]]
        {
            return {nullptr, nullptr};
        }

        auto& slab = *mSlabsArray.at(slabId);
        auto& node = slab.array[entityId];

        return {maybeInitialized(node, node.initialized), &slab};
    }

    std::mutex                  mLock;
    std::list<EntitySlab<T>>    mSlabs;
    std::vector<EntitySlab<T>*> mSlabsArray;
    std::list<EntityId<T>>      mFreeEntities;
};

template <typename T>
Entities<T>::Entities()
    : mPimpl(new Impl)
{
}

template <typename T>
Entities<T>::~Entities()
{
    delete mPimpl;
}

template <typename T>
Entities<T>::EntityWithId Entities<T>::allocate()
{
    return mPimpl->allocate();
}

template <typename T>
void Entities<T>::free(EntityId<T> id)
{
    mPimpl->free(id);
}

template <typename T>
T* Entities<T>::operator[](EntityId<T> id)
{
    return mPimpl->at(id);
}

template struct Entities<Buffer>;

}  // namespace core

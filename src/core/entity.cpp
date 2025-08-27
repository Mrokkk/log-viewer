#include "entity.hpp"

#include <bit>
#include <cstdint>
#include <list>
#include <mutex>

#include "core/view.hpp"
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
        std::scoped_lock lock(lock_);

        if (freeEntities_.empty()) [[unlikely]]
        {
            auto newSlabId = slabs_.size();
            auto& slab = slabs_.emplace_back(newSlabId * EntitySlab<T>::count);
            slabsArray_.emplace_back(&slab);
            for (auto& node : slab.array)
            {
                freeEntities_.push_back(node.id);
            }
        }

        auto id = freeEntities_.front();
        freeEntities_.pop_front();

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
        std::scoped_lock lock(lock_);

        auto nodeAndSlab = getNodeAndSlab(id);

        if (not nodeAndSlab.node) [[unlikely]]
        {
            return;
        }

        auto& node = *nodeAndSlab.node;

        utils::destroyAt(&node.object);
        utils::constructAt(&node.object);
        node.initialized = false;
        freeEntities_.push_back(node.id);
    }

    T* at(EntityId<T> id)
    {
        std::scoped_lock lock(lock_);

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

        auto& slab = *slabsArray_.at(slabId);
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

        if (slabId >= slabsArray_.size()) [[unlikely]]
        {
            return {nullptr, nullptr};
        }

        auto& slab = *slabsArray_.at(slabId);
        auto& node = slab.array[entityId];

        return {maybeInitialized(node, node.initialized), &slab};
    }

    std::mutex                  lock_;
    std::list<EntitySlab<T>>    slabs_;
    std::vector<EntitySlab<T>*> slabsArray_;
    std::list<EntityId<T>>      freeEntities_;
};

template <typename T>
Entities<T>::Entities()
    : pimpl_(new Impl)
{
}

template <typename T>
Entities<T>::~Entities()
{
    delete pimpl_;
}

template <typename T>
Entities<T>::EntityWithId Entities<T>::allocate()
{
    return pimpl_->allocate();
}

template <typename T>
void Entities<T>::free(EntityId<T> id)
{
    pimpl_->free(id);
}

template <typename T>
T* Entities<T>::operator[](EntityId<T> id)
{
    return pimpl_->at(id);
}

template struct Entities<View>;

}  // namespace core

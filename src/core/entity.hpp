#pragma once

#include <utility>

#include "core/entity_id.hpp"
#include "utils/immobile.hpp"

namespace core
{

template <typename T>
struct Entities final : utils::Immobile
{
    using EntityWithId = std::pair<T&, EntityId<T>>;

    Entities();
    ~Entities();
    EntityWithId allocate();
    void free(EntityId<T> id);
    T* operator[](EntityId<T> id);

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace core

#pragma once

#include <cstdint>

namespace core
{

template <typename T>
struct EntityId final
{
    uint32_t index:24;
    uint32_t generation:8;
};

}  // namespace core

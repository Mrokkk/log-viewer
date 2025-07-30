#pragma once

#include <stddef.h>

namespace core
{

struct Mapping
{
    void*  ptr;
    size_t offset;
    size_t len;

    operator bool() const
    {
        return ptr != nullptr;
    }

    template <typename T>
    T ptrAt(size_t off)
    {
        return static_cast<T>(static_cast<const char*>(ptr) + off - offset);
    }
};

}  // namespace core

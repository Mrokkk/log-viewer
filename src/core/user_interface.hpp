#pragma once

#include "core/fwd.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;

    template <typename T>
    T& cast()
    {
        return static_cast<T&>(*this);
    }
};

}  // namespace core

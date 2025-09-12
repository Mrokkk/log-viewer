#pragma once

#include "core/fwd.hpp"
#include "core/mode.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;
    virtual void onModeSwitch(Mode, Context&) = 0;

    template <typename T>
    T& cast()
    {
        return static_cast<T&>(*this);
    }
};

}  // namespace core

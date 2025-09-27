#pragma once

#include "core/event.hpp"
#include "core/input.hpp"

namespace core::events
{

struct KeyPress : Event
{
    constexpr KeyPress(core::KeyPress k)
        : Event(Type::KeyPress)
        , keyPress(k)
    {
    }

    core::KeyPress keyPress;
};

}  // namespace core::events

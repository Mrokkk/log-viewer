#pragma once

#include "core/event.hpp"

namespace core::events
{

struct Resize : Event
{
    constexpr Resize(int x, int y)
        : Event(Type::Resize)
        , resx(x)
        , resy(y)
    {
    }

    int resx;
    int resy;
};

}  // namespace core::events

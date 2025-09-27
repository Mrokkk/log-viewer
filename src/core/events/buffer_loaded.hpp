#pragma once

#include "core/buffer.hpp"
#include "core/event.hpp"
#include "core/window_node.hpp"

namespace core::events
{

struct BufferLoaded : Event
{
    constexpr BufferLoaded(TimeOrError r, WindowNode& n)
        : Event(Type::BufferLoaded)
        , result(r)
        , node(n)
    {
    }

    TimeOrError result;
    WindowNode& node;
};

}  // namespace core::events

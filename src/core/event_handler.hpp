#pragma once

#include "core/event.hpp"

namespace core
{

using EventHandler = std::function<void(EventPtr, InputSource, Context&)>;

void registerEventHandler(Event::Type type, EventHandler handler);

}  // namespace core

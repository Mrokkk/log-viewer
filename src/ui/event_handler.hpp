#pragma once

#include <expected>
#include <functional>
#include <initializer_list>
#include <utility>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"

namespace ftxui
{
struct Event;
}  // namespace ftxui

namespace ui
{

using EventHandler = std::function<bool(Ftxui& ui, core::Context& context)>;
using EventHandlerPair = std::pair<ftxui::Event, EventHandler>;

struct EventHandlers
{
    EventHandlers(std::initializer_list<EventHandlerPair>&& list);
    ~EventHandlers();

    std::expected<bool, int> handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) const;

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

#include "event_handler.hpp"

#include <expected>
#include <flat_map>
#include <utility>

#include <ftxui/component/event.hpp>

namespace ui
{

struct EventHandlers::Impl
{
    Impl(std::initializer_list<EventHandlerPair>&& list)
        : handlers(std::move(list))
    {
    }

    std::expected<bool, int> handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) const
    {
        auto it = handlers.find(event);

        if (it == handlers.end())
        {
            return std::unexpected(0);
        }

        return it->second(ui, context);
    }

    std::flat_map<ftxui::Event, EventHandler> handlers;
};

EventHandlers::EventHandlers(std::initializer_list<EventHandlerPair>&& list)
    : pimpl_(new Impl(std::move(list)))
{
}

EventHandlers::~EventHandlers()
{
    delete pimpl_;
}

std::expected<bool, int> EventHandlers::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) const
{
    return pimpl_->handleEvent(event, ui, context);
}

}  // namespace ui

#include "event_handler.hpp"

#include <flat_map>
#include <utility>

#include <ftxui/component/event.hpp>

#include "utils/maybe.hpp"

namespace ui
{

struct EventHandlers::Impl
{
    Impl(std::initializer_list<EventHandlerPair>&& list)
        : handlers(std::move(list))
    {
    }

    utils::Maybe<bool> handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) const
    {
        auto it = handlers.find(event);

        if (it == handlers.end())
        {
            return {};
        }

        return it->second(ui, context);
    }

    std::flat_map<ftxui::Event, EventHandler> handlers;
};

EventHandlers::EventHandlers(std::initializer_list<EventHandlerPair>&& list)
    : mPimpl(new Impl(std::move(list)))
{
}

EventHandlers::~EventHandlers()
{
    delete mPimpl;
}

utils::Maybe<bool> EventHandlers::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) const
{
    return mPimpl->handleEvent(event, ui, context);
}

}  // namespace ui

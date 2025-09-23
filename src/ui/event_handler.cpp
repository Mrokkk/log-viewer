#include "event_handler.hpp"

#include <utility>

#include <ftxui/component/event.hpp>

#include "ui/event_hash.hpp"
#include "utils/hash_map.hpp"
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
        auto node = handlers.find(event);

        if (not node) [[unlikely]]
        {
            return {};
        }

        return node->second(ui, context);
    }

    utils::HashMap<ftxui::Event, EventHandler> handlers;
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

#include "event.hpp"

#include <functional>

#include "core/context.hpp"
#include "core/event_handler.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/main_loop.hpp"
#include "utils/buffer.hpp"

namespace core
{

using EventHandlers = std::vector<EventHandler>;

static EventHandlers eventHandlers[static_cast<int>(Event::Type::_Size)];

utils::Buffer& operator<<(utils::Buffer& buf, Event::Type type)
{
#define PRINT(NAME) \
    case Event::Type::NAME: return buf << #NAME
    switch (type)
    {
        PRINT(BufferLoaded);
        PRINT(SearchFinished);
        PRINT(KeyPress);
        PRINT(Resize);
        case Event::Type::_Size:
            break;
    }
    return buf << "unexpected<" << static_cast<int>(type) << '>';
}

static void handleEvent(EventPtr event, InputSource source, Context& context)
{
    auto& handlers = eventHandlers[static_cast<int>(event->type())];
    if (not handlers.empty()) [[likely]]
    {
        for (const auto& handler : handlers)
        {
            handler(event, source, context);
        }
    }
    else
    {
        logger.error() << "unhandled event: " << event->type();
    }
    Event::destroy(event);
}

void sendEvent(EventPtr event, InputSource source, Context& context)
{
    if (not context.running) [[unlikely]]
    {
        return;
    }

    context.mainLoop->executeTask(
        [event, source, &context]
        {
            handleEvent(event, source, context);
        });
}

void registerEventHandler(Event::Type type, EventHandler handler)
{
    eventHandlers[static_cast<int>(type)].emplace_back(std::move(handler));
}

}  // namespace core

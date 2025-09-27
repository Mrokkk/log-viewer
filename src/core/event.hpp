#pragma once

#include <cstdint>

#include "core/fwd.hpp"
#include "core/input.hpp"

namespace core
{

struct Event;

using EventPtr = Event*;

struct Event
{
    enum class Type : uint8_t
    {
        BufferLoaded,
        SearchFinished,
        KeyPress,
        Resize,
        _Size
    };

protected:
    constexpr Event(Type type)
        : mType(type)
    {
    }

public:
    constexpr virtual ~Event() = default;

    constexpr Type type() const { return mType; }

    template <typename T>
    constexpr const T& cast() const { return *static_cast<const T*>(this); }

    template <typename T, typename ...Args>
    constexpr static EventPtr create(Args&&... args)
    {
        return new T(std::forward<Args>(args)...);
    }

    constexpr static void destroy(EventPtr event)
    {
        delete event;
    }

private:
    const Type mType;
};

void sendEvent(EventPtr event, InputSource source, Context& context);

template <typename T, typename ...Args>
constexpr void sendEvent(InputSource source, Context& context, Args&&... args)
{
    sendEvent(Event::create<T>(std::forward<Args>(args)...), source, context);
}

}  // namespace core

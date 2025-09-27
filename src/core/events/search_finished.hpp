#pragma once

#include <string>

#include "core/buffer.hpp"
#include "core/event.hpp"
#include "core/window.hpp"

namespace core::events
{

struct SearchFinished : Event
{
    constexpr SearchFinished(SearchResult r, std::string p, Window& w, Buffer& b, float t)
        : Event(Type::SearchFinished)
        , result(r)
        , pattern(p)
        , window(w)
        , buffer(b)
        , time(t)
    {
    }

    SearchResult result;
    std::string pattern;
    Window& window;
    Buffer& buffer;
    float time;
};

}  // namespace core::events

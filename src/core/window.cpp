#include "window.hpp"

#include "core/buffers.hpp"
#include "core/context.hpp"

namespace core
{

Window::Window()
    : initialized(false)
    , loaded(false)
    , pendingSearch(false)
    , lineCount(0)
    , width(0)
    , height(0)
    , lineNrDigits(0)
    , yoffset(0)
    , xoffset(0)
    , ycurrent(0)
    , xcurrent(0)
    , selectionMode(false)
    , selectionPivot(0)
    , selectionStart(0)
    , selectionEnd(0)
    , context(nullptr)
    , config(nullptr)
{
}

Window::Window(BufferId id, Context& c)
    : initialized(true)
    , loaded(false)
    , pendingSearch(false)
    , bufferId(id)
    , lineCount(0)
    , width(0)
    , height(0)
    , lineNrDigits(0)
    , yoffset(0)
    , xoffset(0)
    , ycurrent(0)
    , xcurrent(0)
    , selectionMode(false)
    , selectionPivot(0)
    , selectionStart(0)
    , selectionEnd(0)
    , context(&c)
    , config(&c.config)
{
}

}  // namespace core

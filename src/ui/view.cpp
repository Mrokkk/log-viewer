#define LOG_HEADER "ui::View"
#include "view.hpp"

#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#include "core/views.hpp"
#include "ui/view_renderer.hpp"

using namespace ftxui;

namespace ui
{

View::View(std::string name, core::ViewId viewDataId, const Config& cfg)
    : ViewNode(ViewNode::Type::view, std::move(name))
    , loaded(false)
    , dataId(viewDataId)
    , viewHeight(0)
    , lineNrDigits(0)
    , yoffset(0)
    , xoffset(0)
    , ycurrent(0)
    , xcurrent(0)
    , selectionMode(false)
    , selectionPivot(0)
    , selectionStart(0)
    , selectionEnd(0)
    , config(cfg)
    , ringBuffer(0)
    , viewRenderer(std::make_shared<ViewRenderer>(*this))
{
}

ViewPtr View::create(std::string name, core::ViewId viewDataId, const Config& config)
{
    return std::make_shared<View>(std::move(name), viewDataId, config);
}

}  // namespace ui

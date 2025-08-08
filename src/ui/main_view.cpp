#include "main_view.hpp"

namespace ui
{

MainView::MainView()
    : root(ViewNode::Type::link, "main")
    , currentView(nullptr)
    , activeLine(0)
{
}

}  // namespace ui

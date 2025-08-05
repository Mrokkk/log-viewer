#pragma once

#include "core/fwd.hpp"
#include "ui/ftxui.hpp"

namespace ui
{

bool scrollLeft(Ftxui& ui, core::Context& context);
bool scrollRight(Ftxui& ui, core::Context& context);
bool scrollLineDown(Ftxui& ui, core::Context& context);
bool scrollLineUp(Ftxui& ui, core::Context& context);
bool scrollPageDown(Ftxui& ui, core::Context& context);
bool scrollPageUp(Ftxui& ui, core::Context& context);
bool scrollToEnd(Ftxui& ui, core::Context& context);
void scrollTo(Ftxui& ui, ssize_t lineNumber, core::Context& context);

}  // namespace ui

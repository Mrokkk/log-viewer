#pragma once

#include "core/input.hpp"
#include "utils/maybe.hpp"

namespace ftxui
{
struct Event;
}  // namespace ftxui

namespace ui
{

utils::Maybe<core::KeyPress> convertEvent(const ftxui::Event& event);
void initEventConverter();

}  // namespace ui

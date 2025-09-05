#pragma once

#include <expected>

#include "core/input.hpp"

namespace ftxui
{
struct Event;
}  // namespace ftxui

namespace ui
{

std::expected<core::KeyPress, bool> convertEvent(const ftxui::Event& event);
void initEventConverter();

}  // namespace ui

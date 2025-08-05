#pragma once

#include "core/fwd.hpp"
#include "ftxui.hpp"

namespace ui
{

void createPicker(Ftxui& ui, core::Context& context);
void loadPicker(Ftxui& ui, Picker::Type type, core::Context& context);
void showPicker(Ftxui& ui, Picker::Type type, core::Context& context);
void pickerAccept(Ftxui& ui, core::Context& context);
void pickerNext(Ftxui& ui, core::Context& context);
void pickerPrev(Ftxui& ui, core::Context& context);
ftxui::Element renderPickerWindow(Ftxui& ui, core::Context&);

}  // namespace ui

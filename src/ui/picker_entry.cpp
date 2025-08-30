#include "picker_entry.hpp"

#include <string>

#include <ftxui/dom/elements.hpp>

#include "ui/palette.hpp"

namespace ui
{

ftxui::Element renderPickerEntry(const std::string& entry, bool active)
{
    if (active)
    {
        return ftxui::text("▌" + entry) | color(Palette::Picker::activeLineMarker) | ftxui::bold;
    }
    else
    {
        return ftxui::text(" " + entry);
    }
}

}  // namespace ui

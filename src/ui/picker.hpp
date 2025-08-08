#pragma once

#include <ftxui/component/component_base.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace ui
{

struct Picker final : utils::Immobile
{
    enum Type
    {
        files,
        views,
        commands,
        variables,
        _last
    };

    utils::Strings    strings;
    utils::StringRefs cachedStrings;
    std::string       inputLine;
    ftxui::Component  window;
    ftxui::Component  input;
    ftxui::Component  content;
    ftxui::Component  tabs;
    Type              active;
};

void createPicker(Ftxui& ui, core::Context& context);
void loadPicker(Ftxui& ui, Picker::Type type, core::Context& context);
void showPicker(Ftxui& ui, Picker::Type type, core::Context& context);
void pickerAccept(Ftxui& ui, core::Context& context);
void pickerNext(Ftxui& ui, core::Context& context);
void pickerPrev(Ftxui& ui, core::Context& context);
ftxui::Element renderPickerWindow(Ftxui& ui, core::Context&);

}  // namespace ui

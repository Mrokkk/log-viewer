#pragma once

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "utils/immobile.hpp"

namespace ui
{

struct Grepper : utils::Immobile
{
    struct State
    {
        std::string   pattern;
        bool          regex           = false;
        bool          caseInsensitive = false;
        bool          inverted        = false;
    };

    State             state;
    ftxui::Component  input;
    ftxui::Components checkboxes;
    ftxui::Component  window;
};

void createGrepper(Ftxui& ui);
void grepperAccept(Ftxui& ui, core::Context& context);
ftxui::Element renderGrepper(Ftxui& ui);

}  // namespace ui

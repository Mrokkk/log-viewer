#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct MainView final : UIComponent
{
    MainView();
    ~MainView();

    ftxui::Element render(core::Context& context) override;

    operator ftxui::Component&();

private:
    ftxui::Component mPlaceholder;

    ftxui::Elements renderTablines(core::Context& context);
};

}  // namespace ui

#pragma once

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

private:
    ftxui::Elements renderTablines(core::Context& context);
};

}  // namespace ui

#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct MainView final : UIComponent
{
    MainView();
    ~MainView();

    void takeFocus() override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    ftxui::Element render(core::Context& context) override;

    operator ftxui::Component&();

private:
    ftxui::Component mPlaceholder;

    ftxui::Elements renderTablines(core::Context& context);
};

}  // namespace ui

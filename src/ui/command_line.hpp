#pragma once

#include <ftxui/fwd.hpp>

#include "ui/text_box.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct CommandLine final : UIComponent
{
    CommandLine(Ftxui& ui, core::Context& context);
    ~CommandLine();
    void takeFocus() override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    ftxui::Element render(core::Context& context) override;

private:
    TextBox mTextBox;
};

}  // namespace ui

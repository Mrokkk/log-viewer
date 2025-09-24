#pragma once

#include <ftxui/fwd.hpp>

#include "ui/text_box.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct CommandLine final : UIComponent
{
    CommandLine(core::Context& context);
    ~CommandLine();
    ftxui::Element render(core::Context& context) override;

private:
    TextBox mCommandTextBox;
    TextBox mSearchTextBox;
};

}  // namespace ui

#pragma once

#include <string_view>

#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/text_box.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Grepper final : UIComponent
{
    Grepper(core::Context& context);
    ~Grepper();
    ftxui::Element render(core::Context& context) override;

private:
    ftxui::Element renderCheckbox(bool value, std::string_view description);

    TextBox mTextBox;
};

}  // namespace ui

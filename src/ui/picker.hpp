#pragma once

#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/text_box.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Picker final : UIComponent
{
    Picker(core::Context& context);
    ~Picker();
    ftxui::Element render(core::Context& context) override;

private:
    TextBox           mTextBox;
    ftxui::Component  mTabs;
    ftxui::Component  mPlaceholder;
    ftxui::Component  mWindow;
};

}  // namespace ui

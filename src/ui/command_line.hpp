#pragma once

#include <ftxui/fwd.hpp>

#include "ui/ui_component.hpp"

namespace ui
{

struct CommandLine final : UIComponent
{
    CommandLine(core::Context& context);
    ~CommandLine();
    void takeFocus() override;
    ftxui::Element render(core::Context& context) override;
    operator ftxui::Component&();

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

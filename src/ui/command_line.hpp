#pragma once

#include <ftxui/fwd.hpp>

#include "core/severity.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct CommandLine final : UIComponent
{
    CommandLine(core::Context& context);
    ~CommandLine();
    void takeFocus() override;
    ftxui::Element render(core::Context& context) override;
    void clearMessageLine();
    std::ostream& operator<<(Severity severity);
    operator ftxui::Component&();

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

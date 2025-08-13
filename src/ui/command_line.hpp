#pragma once

#include <ftxui/fwd.hpp>

#include "core/logger.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct CommandLine final : UIComponent
{
    CommandLine();
    ~CommandLine();
    void takeFocus() override;
    ftxui::Element render(core::Context& context) override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    void clearMessageLine();
    void clearInputLine();
    std::ostream& operator<<(Severity severity);
    operator ftxui::Component&();

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

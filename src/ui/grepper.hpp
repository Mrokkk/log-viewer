#pragma once

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Grepper final : UIComponent
{
    Grepper();
    ~Grepper();
    void takeFocus() override;
    ftxui::Element render(core::Context& context) override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    operator ftxui::Component&();

private:
    void accept(Ftxui& ui, core::Context& context);
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

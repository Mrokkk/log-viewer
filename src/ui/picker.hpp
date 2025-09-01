#pragma once

#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Picker final : UIComponent
{
    enum Type : int
    {
        files,
        views,
        commands,
        variables,
        messages,
        logs,
        _last
    };

    Picker();
    ~Picker();
    void onExit() override;
    void takeFocus() override;
    ftxui::Element render(core::Context& context) override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    void show(Ftxui& ui, Picker::Type type, core::Context& context);
    operator ftxui::Component&();

private:
    struct Impl;
    Impl* mPimpl;
};

}  // namespace ui

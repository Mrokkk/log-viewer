#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "utils/immobile.hpp"

namespace ui
{

struct UIComponent : utils::Immobile
{
    enum Type
    {
        mainView,
        commandLine,
        picker,
        grepper,
    };

    UIComponent(Type t);
    virtual ~UIComponent();

    virtual void onExit();
    virtual void takeFocus();
    virtual bool handleEvent(const ftxui::Event&, Ftxui&, core::Context&);

    virtual ftxui::Element render(core::Context&) = 0;

    const Type type;
};

}  // namespace ui

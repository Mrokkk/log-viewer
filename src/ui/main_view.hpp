#pragma once

#include <ftxui/fwd.hpp>

#include "core/context.hpp"
#include "core/fwd.hpp"
#include "core/views.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"
#include "ui/view.hpp"

namespace ui
{

struct MainView final : UIComponent
{
    MainView();
    ~MainView();

    void takeFocus() override;
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context) override;
    ftxui::Element render(core::Context& context) override;
    void reload(Ftxui& ui, core::Context& context);
    ViewNodePtr createView(std::string name, core::ViewId viewDataId, ViewNode* parent = nullptr);
    void removeView(ViewNode& view, core::Context& context);
    void scrollTo(Ftxui& ui, long lineNumber, core::Context& context);
    const char* activeFileName(core::Context& context) const;
    bool isViewLoaded() const;
    View* currentView();
    ViewNode& root();

    operator ftxui::Component&();

private:
    struct Impl;
    Impl* mPimpl;
};

}  // namespace ui

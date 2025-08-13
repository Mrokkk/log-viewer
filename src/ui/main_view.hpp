#pragma once

#include <ftxui/fwd.hpp>

#include "core/fwd.hpp"
#include "ui/fwd.hpp"
#include "ui/ui_component.hpp"

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
    ViewNode& createView(std::string name, ViewNode* parent = nullptr);
    void scrollTo(Ftxui& ui, ssize_t lineNumber, core::Context& context);
    const char* activeFileName() const;
    bool isViewLoaded() const;
    View* currentView();
    ViewNode& root();

    operator ftxui::Component&();

private:
    struct Impl;
    Impl* pimpl_;
};

}  // namespace ui

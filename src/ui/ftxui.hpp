#pragma once

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/user_interface.hpp"
#include "ui/command_line.hpp"
#include "ui/event_handler.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/picker.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Ftxui final : core::UserInterface
{
    Ftxui();
    void run(core::Context& context) override;
    void quit(core::Context& context) override;
    void executeShell(const std::string& command) override;
    void scrollTo(ssize_t lineNumber, core::Context& context) override;
    std::ostream& operator<<(Severity severity) override;
    void* createView(std::string name, core::Context& context) override;
    void attachFileToView(core::File& file, void* view, core::Context& context) override;

    ftxui::ScreenInteractive screen;
    ftxui::Dimensions        terminalSize;
    bool                     showLineNumbers;
    UIComponent*             active;
    MainView                 mainView;
    CommandLine              commandLine;
    Picker                   picker;
    Grepper                  grepper;
    EventHandlers            eventHandlers;
};

void switchFocus(UIComponent::Type element, Ftxui& ui, core::Context& context);
core::UserInterface& createFtxuiUserInterface(core::Context& context);

}  // namespace ui

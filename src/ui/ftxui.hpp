#pragma once

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/main_loop.hpp"
#include "core/user_interface.hpp"
#include "ui/command_line.hpp"
#include "ui/event_handler.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/picker.hpp"
#include "ui/ui_component.hpp"

namespace ui
{

struct Ftxui final : core::UserInterface, core::MainLoop
{
    Ftxui(core::Context& context);

    void onModeSwitch(core::Mode newMode, core::Context& context) override;

    void run(core::Context& context) override;
    void quit(core::Context& context) override;
    void executeShell(const std::string& command) override;
    void executeTask(core::Task task) override;

    ftxui::ScreenInteractive screen;
    ftxui::Dimensions        terminalSize;
    UIComponent*             active;
    MainView                 mainView;
    CommandLine              commandLine;
    Picker                   picker;
    Grepper                  grepper;
    EventHandlers            eventHandlers;
};

void switchFocus(UIComponent::Type element, Ftxui& ui, core::Context& context);
void createFtxuiUserInterface(core::Context& context);

}  // namespace ui

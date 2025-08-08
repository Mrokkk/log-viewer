#pragma once

#include <functional>
#include <map>
#include <spanstream>
#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/mapped_file.hpp"
#include "core/user_interface.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/picker.hpp"
#include "utils/immobile.hpp"

namespace ui
{

struct Ftxui;

using EventHandler = std::function<bool(Ftxui& ui, core::Context& context)>;
using EventHandlers = std::map<ftxui::Event, EventHandler>;

enum UIElement
{
    logView,
    commandLine,
    picker,
    grepper,
};

struct CommandLine final : utils::Immobile
{
    CommandLine();

    Severity         severity;
    char             buffer[256];
    std::string      inputLine;
    std::ospanstream messageLine;
    ftxui::Component input;
};

struct Ftxui final : core::UserInterface
{
    Ftxui();
    void run(core::Context& context) override;
    void quit(core::Context& context) override;
    void executeShell(const std::string& command) override;
    void scrollTo(ssize_t lineNumber, core::Context& context) override;
    std::ostream& operator<<(Severity severity) override;
    void* createView(std::string name, core::Context& context) override;
    void attachFileToView(core::MappedFile& file, void* view, core::Context& context) override;

    ftxui::ScreenInteractive screen;
    ftxui::Dimensions        terminalSize;
    bool                     showLineNumbers;
    MainView                 mainView;
    CommandLine              commandLine;
    Picker                   picker;
    Grepper                  grepper;
    UIElement                active;
    EventHandlers            eventHandlers;
};

core::UserInterface& createFtxuiUserInterface(core::Context& context);

}  // namespace ui

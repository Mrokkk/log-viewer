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
#include "ui/view.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

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

struct Picker final : utils::Immobile
{
    enum Type
    {
        files,
        views,
        commands,
        variables,
        _last
    };

    utils::Strings    strings;
    utils::StringRefs cachedStrings;
    std::string       inputLine;
    ftxui::Component  window;
    ftxui::Component  input;
    ftxui::Component  content;
    ftxui::Component  tabs;
    Type              active;
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
    ViewNode                 mainView;
    View*                    currentView;
    int                      activeLine;
    CommandLine              commandLine;
    Picker                   picker;
    Grepper                  grepper;
    UIElement                active;
    EventHandlers            eventHandlers;
};

core::UserInterface& createFtxuiUserInterface(core::Context& context);

bool isViewLoaded(Ftxui& ui);
void reloadLines(View& view, core::Context& context);
std::string getLine(View& view, size_t lineIndex, core::Context& context);
void reloadView(View& view, Ftxui& ui, core::Context& context);

}  // namespace ui

#pragma once

#include <string>

#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/fwd.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/fwd.hpp"
#include "core/main_loop.hpp"
#include "core/user_interface.hpp"

namespace ui
{

struct Ftxui;

struct MainView final
{
    ftxui::Element render(core::Context& context);
private:
    ftxui::Elements renderTablines(core::Context& context);
};

struct TextBox
{
    const std::string&  content;
    const size_t*       cursorPosition = nullptr;
    const std::string*  suggestion = nullptr;
    const ftxui::Color* suggestionColor = nullptr;
};

struct CommandLine final
{
    CommandLine(core::Context& context);
    ftxui::Element render(core::Context& context);

private:
    TextBox mCommandTextBox;
    TextBox mSearchTextBox;
};

struct MainPicker final
{
    MainPicker(core::Context& context);
    ftxui::Element render(core::Context& context);

private:
    TextBox           mTextBox;
};

struct Grepper final
{
    Grepper(core::Context& context);
    ftxui::Element render(core::Context& context);

private:
    ftxui::Element renderCheckbox(bool value, std::string_view description);

    TextBox mTextBox;
};

struct Ftxui final : core::UserInterface, core::MainLoop
{
    Ftxui(core::Context& context);

    void run(core::Context& context) override;
    void quit(core::Context& context) override;
    void executeShell(const std::string& command) override;
    void executeTask(core::Task task) override;

    ftxui::ScreenInteractive screen;
    ftxui::Dimensions        terminalSize;
    MainView                 mainView;
    CommandLine              commandLine;
    MainPicker               picker;
    Grepper                  grepper;
};

}  // namespace ui

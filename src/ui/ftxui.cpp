#define LOG_HEADER "ui::Ftxui"
#include "ftxui.hpp"

#include <cstdlib>
#include <expected>
#include <memory>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/input.hpp"
#include "core/mode.hpp"
#include "core/thread.hpp"
#include "sys/system.hpp"
#include "ui/command_line.hpp"
#include "ui/event_converter.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/palette.hpp"
#include "ui/picker.hpp"
#include "ui/status_line.hpp"

using namespace ftxui;

namespace ui
{

static bool abort(Ftxui&, core::Context&)
{
    std::abort();
    return true;
}

static bool resize(Ftxui& ui, core::Context& context)
{
    ui.terminalSize = Terminal::Size();
    core::resize(ui.terminalSize.dimx, ui.terminalSize.dimy, context);
    return true;
}

static bool ctrlC(Ftxui&, core::Context& context)
{
    core::registerKeyPress(core::KeyPress::ctrl('c'), core::InputSource::user, context);
    return true;
}

static bool ctrlZ(Ftxui&, core::Context& context)
{
    core::registerKeyPress(core::KeyPress::ctrl('z'), core::InputSource::user, context);
    return true;
}

static Element render(Ftxui& ui, core::Context& context)
{
    auto view = ui.mainView.render(context);
    auto mode = context.mode;

    if (mode == core::Mode::picker or mode == core::Mode::grepper or context.inputState.assistedMode)
    {
        Elements overlays;
        overlays.reserve(4);

        overlays.emplace_back(std::move(view));

        switch (context.mode)
        {
            case core::Mode::picker:
                overlays.emplace_back(ui.picker.render(context));
                break;
            case core::Mode::grepper:
                overlays.emplace_back(ui.grepper.render(context));
                break;
            default:
                break;
        }

        if (context.inputState.assistedMode)
        {
            Elements left;
            Elements right;

            left.reserve(context.inputState.helpEntries.size());
            right.reserve(context.inputState.helpEntries.size());

            for (const auto& entry : context.inputState.helpEntries)
            {
                left.emplace_back(text(&entry.name));
                right.emplace_back(text(&entry.help) | align_right);
            }

            overlays.emplace_back(
                vbox(
                    filler(),
                    hbox(
                        filler(),
                        window(
                            text("Help for " + core::inputStateString(context)),
                            hbox(
                                vbox(std::move(left)) | color(Palette::fg1),
                                filler(),
                                text(" "),
                                vbox(std::move(right) | align_right) | color(Palette::fg3)),
                            LIGHT)
                                | color(Palette::fg0)
                                | clear_under
                                | size(WIDTH, GREATER_THAN, ui.terminalSize.dimx / 4)
                                | size(HEIGHT, GREATER_THAN, ui.terminalSize.dimy / 3)
                        )));
        }

        view = dbox(std::move(overlays)) | flex;
    }

    return vbox(
        std::move(view),
        renderStatusLine(ui, context),
        ui.commandLine.render(context))
            | flex;
}

static bool handleEvent(const Event& event, Ftxui& ui, core::Context& context)
{
    auto keyPress = convertEvent(event);

    if (keyPress)
    {
        if (core::registerKeyPress(*keyPress, core::InputSource::user, context))
        {
            return true;
        }
    }

    auto result = ui.eventHandlers.handleEvent(event, ui, context);

    if (result and *result)
    {
        return true;
    }

    return false;
}

static inline ScreenInteractive createScreen(Ftxui& ui, core::Context& context)
{
    resize(ui, context);
    return ScreenInteractive::Fullscreen();
}

Ftxui::Ftxui(core::Context& context)
    : screen(createScreen(*this, context))
    , commandLine(context)
    , picker(context)
    , grepper(context)
    , eventHandlers{
        {Event::Resize, resize},
        {Event::F12,    abort},
        {Event::CtrlC,  ctrlC},
        {Event::CtrlZ,  ctrlZ},
    }
{
    initEventConverter();
}

void Ftxui::run(core::Context& context)
{
    auto component
        = Renderer(
            [&context, this]
            {
                return render(*this, context);
            })
        | CatchEvent(
            [&context, this](const Event& event)
            {
                return handleEvent(event, *this, context);
            });

    resize(*this, context);

    screen
        .OnCrash([](const int signal){ sys::crashHandle(signal); })
        .ForceHandleCtrlC(false)
        .ForceHandleCtrlZ(false)
        .Loop(std::move(component));
}

void Ftxui::quit(core::Context&)
{
    screen.Exit();
}

void Ftxui::executeShell(const std::string& cmd)
{
    auto command = cmd + "; read -n 1 -s -r -p \"\nCommand exited with $?; press any key to continue\"; echo";
    screen.Post(screen.WithRestoredIO(
        [command = std::move(command)]
        {
            std::system(command.c_str());
        }));
}

void Ftxui::executeTask(core::Task task)
{
    if (core::isMainThread())
    {
        task();
    }
    else
    {
        screen.Post(
            [task = std::move(task)]
            {
                task();
            });
        screen.RequestAnimationFrame();
    }
}

void createFtxuiUserInterface(core::Context& context)
{
    auto ui = new Ftxui(context);
    context.ui = ui;
    context.mainLoop = ui;
}

}  // namespace ui

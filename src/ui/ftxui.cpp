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
#include "core/main_view.hpp"
#include "core/mode.hpp"
#include "sys/system.hpp"
#include "ui/command_line.hpp"
#include "ui/event_converter.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/picker.hpp"
#include "ui/status_line.hpp"
#include "ui/ui_component.hpp"

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
    context.mainView.resize(ui.terminalSize.dimx, ui.terminalSize.dimy, context);
    return false; // Allow UIComponent to handle it as well
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

    Element overlay;

    switch (ui.active->type)
    {
        case UIComponent::picker:
            overlay = ui.picker.render(context);
            break;
        case UIComponent::grepper:
            overlay = ui.grepper.render(context);
            break;
        default:
            break;
    }

    if (overlay)
    {
        view = dbox(
            std::move(view),
            std::move(overlay)) | flex;
    }

    auto children = Elements{
        std::move(view),
        renderStatusLine(ui, context),
        ui.commandLine.render(context)
    };

    return vbox(std::move(children)) | flex;
}

static bool handleEvent(const Event& event, Ftxui& ui, core::Context& context)
{
    if (context.mode != core::Mode::custom)
    {
        auto keyPress = convertEvent(event);

        if (keyPress)
        {
            if (core::registerKeyPress(*keyPress, core::InputSource::user, context))
            {
                return true;
            }
        }
    }

    auto result = ui.eventHandlers.handleEvent(event, ui, context);

    if (result and result.value())
    {
        return true;
    }

    return ui.active->handleEvent(event, ui, context);
}

static inline ScreenInteractive createScreen(Ftxui& ui, core::Context& context)
{
    resize(ui, context);
    return ScreenInteractive::Fullscreen();
}

Ftxui::Ftxui(core::Context& context)
    : screen(createScreen(*this, context))
    , active(&mainView)
    , commandLine(*this, context)
    , eventHandlers{
        {Event::Resize, resize},
        {Event::F12,    abort},
        {Event::CtrlC,  ctrlC},
        {Event::CtrlZ,  ctrlZ},
    }
{
    initEventConverter();
}

void Ftxui::onModeSwitch(core::Mode newMode, core::Context& context)
{
    switch (newMode)
    {
        case core::Mode::command:
            switchFocus(UIComponent::commandLine, *this, context);
            break;
        case core::Mode::normal:
            switchFocus(UIComponent::mainView, *this, context);
            break;
        default:
            break;
    }
}

void Ftxui::run(core::Context& context)
{
    auto component
        = Renderer(
            Container::Stacked({
                picker,
                grepper,
                mainView
            }),
            [&context, this]
            {
                return render(*this, context);
            })
        | CatchEvent(
            [&context, this](const Event& event)
            {
                return handleEvent(event, *this, context);
            });

    switchFocus(UIComponent::mainView, *this, context);

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

void Ftxui::executeTask(std::function<void()> closure)
{
    screen.Post(
        [closure = std::move(closure)]
        {
            closure();
        });
    screen.RequestAnimationFrame();
}

void createFtxuiUserInterface(core::Context& context)
{
    auto ui = new Ftxui(context);
    context.ui = ui;
    context.mainLoop = ui;
}

void switchFocus(UIComponent::Type element, Ftxui& ui, core::Context& context)
{
    if (ui.active)
    {
        ui.active->onExit();
    }

    switch (element)
    {
        case UIComponent::commandLine:
            ui.active = &ui.commandLine;
            break;
        case UIComponent::mainView:
            ui.active = &ui.mainView;
            break;
        case UIComponent::picker:
            ui.active = &ui.picker;
            core::switchMode(core::Mode::custom, context);
            break;
        case UIComponent::grepper:
            ui.active = &ui.grepper;
            core::switchMode(core::Mode::custom, context);
            break;
    }

    ui.active->takeFocus();
}

}  // namespace ui

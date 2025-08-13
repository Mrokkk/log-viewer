#include "ftxui.hpp"

#include <cstdlib>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/file.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/mode.hpp"
#include "sys/system.hpp"
#include "ui/command_line.hpp"
#include "ui/grepper.hpp"
#include "ui/main_view.hpp"
#include "ui/picker.hpp"
#include "ui/status_line.hpp"
#include "ui/ui_component.hpp"
#include "ui/view.hpp"

using namespace ftxui;

namespace ui
{

static bool abort(Ftxui&, core::Context&)
{
    std::abort();
    return true;
}

static bool escape(Ftxui& ui, core::Context& context)
{
    core::registerKeyPress('\e', context);
    switchFocus(UIComponent::mainView, ui, context);
    return false; // Allow UIComponent to handle it as well
}

static bool ctrlC(Ftxui& ui, core::Context&)
{
    ui << info << "Type :qa and press <Enter> to quit";
    return true;
}

static bool resize(Ftxui& ui, core::Context&)
{
    ui.terminalSize = Terminal::Size();
    return false; // Allow UIComponent to handle it as well
}

static bool ignore(Ftxui&, core::Context&)
{
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
    logger << debug << event.DebugString();

    auto result = ui.eventHandlers.handleEvent(event, ui, context);

    if (result and result.value())
    {
        return true;
    }

    return ui.active->handleEvent(event, ui, context);
}

Ftxui::Ftxui()
    : screen(ScreenInteractive::Fullscreen())
    , showLineNumbers(false)
    , active(&mainView)
    , eventHandlers{
        {Event::Escape, escape},
        {Event::CtrlC,  ctrlC},
        {Event::CtrlZ,  ignore},
        {Event::Resize, resize},
        {Event::F12,    abort},
    }
{
}

void Ftxui::run(core::Context& context)
{
    auto component
        = Renderer(
            Container::Stacked({
                picker,
                grepper,
                Container::Vertical({mainView, commandLine})
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

    resize(*this, context);
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

void Ftxui::scrollTo(ssize_t lineNumber, core::Context& context)
{
    mainView.scrollTo(*this, lineNumber, context);
}

std::ostream& Ftxui::operator<<(Severity severity)
{
    return commandLine << severity;
}

void* Ftxui::createView(std::string name, core::Context&)
{
    return &mainView.createView(std::move(name));
}

void Ftxui::attachFileToView(core::File& file, void* handle, core::Context& context)
{
    auto& view = *static_cast<ViewNode*>(handle);

    screen.Post(
        [this, &file, &view, &context]
        {
            auto& base = view.base().cast<View>();
            base.file = &file;
            base.lineCount = file.lineCount();

            *this << info
                << file.path() << ": lines: " << file.lineCount() << "; took "
                << std::fixed << std::setprecision(3) << file.loadTime() << " s";

            reloadView(base, *this, context);
        });
    screen.RequestAnimationFrame();
}

core::UserInterface& createFtxuiUserInterface(core::Context& context)
{
    auto ui = std::make_unique<Ftxui>();
    auto& ref = *ui;
    context.ui = std::move(ui);
    return ref;
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
            core::switchMode(core::Mode::command, context);
            break;
        case UIComponent::grepper:
            ui.active = &ui.grepper;
            core::switchMode(core::Mode::custom, context);
            break;
        case UIComponent::mainView:
            ui.active = &ui.mainView;
            core::switchMode(core::Mode::normal, context);
            break;
        case UIComponent::picker:
            ui.active = &ui.picker;
            core::switchMode(core::Mode::custom, context);
            break;
    }

    ui.active->takeFocus();
}

}  // namespace ui

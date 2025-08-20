#include "ftxui.hpp"

#include <cstdlib>
#include <flat_map>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/context.hpp"
#include "core/file.hpp"
#include "core/input.hpp"
#include "core/mode.hpp"
#include "core/severity.hpp"
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

static bool resize(Ftxui& ui, core::Context&)
{
    ui.terminalSize = Terminal::Size();
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

static std::flat_map<Event, core::KeyPress> eventToKeyPress;

static bool handleEvent(const Event& event, Ftxui& ui, core::Context& context)
{
    if (context.mode != core::Mode::custom)
    {
        auto it = eventToKeyPress.find(event);

        if (it != eventToKeyPress.end())
        {
            if (core::registerKeyPress(it->second, core::InputSource::user, context))
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

Ftxui::Ftxui(core::Context& context)
    : screen(ScreenInteractive::Fullscreen())
    , showLineNumbers(false)
    , active(&mainView)
    , commandLine(context)
    , eventHandlers{
        {Event::Resize, resize},
        {Event::F12,    abort},
        {Event::CtrlC,  ctrlC},
        {Event::CtrlZ,  ctrlZ},
    }
{
    eventToKeyPress = {
        {Event::Return,         core::KeyPress::cr},
        {Event::Escape,         core::KeyPress::escape},
        {Event::ArrowDown,      core::KeyPress::arrowDown},
        {Event::ArrowUp,        core::KeyPress::arrowUp},
        {Event::ArrowLeft,      core::KeyPress::arrowLeft},
        {Event::ArrowRight,     core::KeyPress::arrowRight},
        {Event::ArrowDownCtrl,  core::KeyPress::ctrlArrowDown},
        {Event::ArrowUpCtrl,    core::KeyPress::ctrlArrowUp},
        {Event::ArrowLeftCtrl,  core::KeyPress::ctrlArrowLeft},
        {Event::ArrowRightCtrl, core::KeyPress::ctrlArrowRight},
        {Event::PageDown,       core::KeyPress::pageDown},
        {Event::PageUp,         core::KeyPress::pageUp},
        {Event::Backspace,      core::KeyPress::backspace},
        {Event::Delete,         core::KeyPress::del},
        {Event::Home,           core::KeyPress::home},
        {Event::End,            core::KeyPress::end},
        {Event::Tab,            core::KeyPress::tab},
        {Event::TabReverse,     core::KeyPress::shiftTab},
        {Event::F1,             core::KeyPress::function(1)},
        {Event::F2,             core::KeyPress::function(2)},
        {Event::F3,             core::KeyPress::function(3)},
        {Event::F4,             core::KeyPress::function(4)},
        {Event::F5,             core::KeyPress::function(5)},
        {Event::F6,             core::KeyPress::function(6)},
        {Event::F7,             core::KeyPress::function(7)},
        {Event::F8,             core::KeyPress::function(8)},
        {Event::F9,             core::KeyPress::function(9)},
        {Event::F10,            core::KeyPress::function(10)},
        {Event::F11,            core::KeyPress::function(11)},
        {Event::F12,            core::KeyPress::function(12)},
        {Event::Character('['), core::KeyPress::character('[')},
        {Event::Character('\\'), core::KeyPress::character('\\')},
        {Event::Character(']'), core::KeyPress::character(']')},
        {Event::Character('^'), core::KeyPress::character('^')},
        {Event::Character('_'), core::KeyPress::character('_')},
        {Event::Character('`'), core::KeyPress::character('`')},
        {Event::Character('{'), core::KeyPress::character('{')},
        {Event::Character('|'), core::KeyPress::character('|')},
        {Event::Character('}'), core::KeyPress::character('}')},
        {Event::Character('~'), core::KeyPress::character('~')},
    };

    for (char c = '@'; c <= 'Z'; ++c)
    {
        char small = c + 0x20;
        char capital = c;
        char ctrl = c - 0x40;

        // Small letter
        eventToKeyPress.emplace(
            std::make_pair(
                Event::Character(std::string{small}),
                core::KeyPress::character(small)));

        // Capital letter
        eventToKeyPress.emplace(
            std::make_pair(
                Event::Character(std::string{capital}),
                core::KeyPress::character(capital)));

        if (ctrl != '\e' and ctrl != '\t')
        {
            // Ctrl+letter
            eventToKeyPress.emplace(
                std::make_pair(
                    Event::Special(std::string{ctrl}),
                    core::KeyPress::ctrl(small)));
        }

        // Alt+letter
        eventToKeyPress.emplace(
            std::make_pair(
                Event::Special(std::string{'\e', small}),
                core::KeyPress::alt(small)));
    }

    for (char c = ' '; c <= '?'; ++c)
    {
        eventToKeyPress.emplace(
            std::make_pair(
                Event::Character(std::string{c}),
                core::KeyPress::character(c)));
    }
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

void Ftxui::scrollTo(core::Scroll lineNumber, core::Context& context)
{
    switch (lineNumber)
    {
        case core::Scroll::end:
            mainView.scrollTo(*this, -1, context);
            break;
        default:
            mainView.scrollTo(*this, static_cast<long>(lineNumber), context);
    }
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

void Ftxui::onModeSwitch(core::Mode newMode, core::Context& context)
{
    switch (newMode)
    {
        case core::Mode::command:
            switchFocus(UIComponent::commandLine, *this, context);
            break;
        case core::Mode::normal:
            if (auto view = mainView.currentView())
            {
                view->selectionMode = false;
            }
            switchFocus(UIComponent::mainView, *this, context);
            break;
        default:
            break;
    }
}

core::UserInterface& createFtxuiUserInterface(core::Context& context)
{
    return *(context.ui = new Ftxui(context));
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

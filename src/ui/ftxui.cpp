#include "ftxui.hpp"

#include <cstdlib>
#include <cstring>
#include <flat_set>
#include <iomanip>
#include <ios>
#include <spanstream>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/flexbox_config.hpp>
#include <ftxui/screen/colored_string.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/command.hpp"
#include "core/context.hpp"
#include "core/logger.hpp"
#include "core/mapped_file.hpp"
#include "core/operations.hpp"
#include "core/variable.hpp"
#include "sys/system.hpp"
#include "ui/picker.hpp"
#include "ui/palette.hpp"
#include "ui/scroll.hpp"
#include "utils/math.hpp"
#include "utils/ring_buffer.hpp"
#include "utils/side_effect.hpp"
#include "utils/string.hpp"

using namespace ftxui;

namespace ui
{

static bool abort(Ftxui&, core::Context&)
{
    std::abort();
    return true;
}

static void clearMessageLine(Ftxui& ui)
{
    memset(ui.commandLine.buffer, 0, sizeof(ui.commandLine.buffer));
    std::ospanstream s(ui.commandLine.buffer);
    ui.commandLine.messageLine.swap(s);
}

static bool escape(Ftxui& ui, core::Context& context)
{
    if (ui.active == UIElement::picker)
    {
        ui.picker.cachedStrings.clear();
        ui.picker.strings.clear();
    }
    clearMessageLine(ui);
    ui.active = UIElement::logView;
    core::registerKeyPress('\e', context);
    return true;
}

static bool ctrlC(Ftxui& ui, core::Context&)
{
    ui << info << "Type :qa and press <Enter> to quit";
    return true;
}

static bool enter(Ftxui& ui, core::Context& context)
{
    switch (ui.active)
    {
        case logView:
        default:
            return false;

        case picker:
            pickerAccept(ui, context);
            return true;

        case commandLine:
            core::executeCode(ui.commandLine.inputLine, context);
            // If command already changed active, do not change it
            if (ui.active == UIElement::commandLine)
            {
                ui.active = UIElement::logView;
            }
            return true;
    }
}

static bool tab(Ftxui& ui, core::Context& context)
{
    if (ui.active != UIElement::picker)
    {
        return false;
    }

    pickerNext(ui, context);
    return true;
}

static bool tabReverse(Ftxui& ui, core::Context& context)
{
    if (ui.active != UIElement::picker)
    {
        return false;
    }

    pickerPrev(ui, context);
    return true;
}

template <typename T>
static EventHandler whenActive(UIElement element, T func)
{
    return
        [element, func](Ftxui& ui, core::Context& context)
        {
            if (context.ui->get<Ftxui>().active == element)
            {
                func(ui, context);
                return true;
            }
            return false;
        };
}

static bool resize(Ftxui& ui, core::Context& context)
{
    auto oldTerminalSize = ui.terminalSize;
    ui.terminalSize = Terminal::Size();

    if (not isViewLoaded(ui))
    {
        return false;
    }

    if (ui.terminalSize.dimy != oldTerminalSize.dimy)
    {
        logger << info << "terminal size: " << ui.terminalSize.dimx << 'x' << ui.terminalSize.dimy;

        ui.mainView.forEachRecursive(
            [&ui, &context](ViewNode& view)
            {
                if (view.type() == ViewNode::Type::view)
                {
                    reloadView(view.cast<View>(), ui, context);
                }
            });
    }

    return true;
}

static bool pageUp(Ftxui& ui, core::Context& context)
{
    scrollPageUp(ui, context);
    return true;
}

static bool pageDown(Ftxui& ui, core::Context& context)
{
    scrollPageDown(ui, context);
    return true;
}

static bool arrowDown(Ftxui& ui, core::Context& context)
{
    scrollLineDown(ui, context);
    return true;
}

static bool arrowUp(Ftxui& ui, core::Context& context)
{
    scrollLineUp(ui, context);
    return true;
}

static bool arrowLeft(Ftxui& ui, core::Context& context)
{
    scrollLeft(ui, context);
    return true;
}

static bool arrowRight(Ftxui& ui, core::Context& context)
{
    return scrollRight(ui, context);
}

static bool ignore(Ftxui&, core::Context&)
{
    return true;
}

static constexpr struct ToFtxuiText{} toFtxuiText;

static Elements operator|(utils::RingBuffer<std::string>& buf, const ToFtxuiText&)
{
    Elements vec;
    buf.forEach([&](const auto& line){ vec.emplace_back(text(&line) | flex); });
    return vec;
}

static Element renderEmptyView(Ftxui& ui, core::Context& context)
{
    if (ui.active == UIElement::picker)
    {
        return dbox(text(""), renderPickerWindow(ui, context)) | flex;
    }

    return vbox({
        text("Hello!") | center,
        text("Type :files<Enter> to open file picker") | center,
        text("Alternatively, type :e <file><Enter> to open given file") | center,
    }) | center | flex;
}

static Element wrapActiveLineIf(Element line, bool condition)
{
    if (not condition)
    {
        return line;
    }
    return hbox({
        line,
        text("")
            | color(Palette::TabLine::inactiveLineBg)
            | bgcolor(Palette::TabLine::activeLineMarker),
        text("")
            | color(Palette::TabLine::activeLineMarker)
            | bgcolor(Palette::TabLine::activeLineBg)
            | xflex,
    });
}

static Elements renderTablines(Ftxui& ui)
{
    Elements vertical;
    vertical.reserve(5);

    int index = 0;

    vertical.push_back(
        wrapActiveLineIf(
            ui.mainView.renderTabline(),
            ui.activeLine == index++));

    auto view = ui.mainView.activeChild();

    while (view->activeChild())
    {
        vertical.push_back(
            wrapActiveLineIf(
                view->renderTabline(),
                ui.activeLine == index++));

        view = view->activeChild();
    }

    return vertical;
}

static Element renderView(Ftxui& ui, core::Context& context)
{
    auto vertical = renderTablines(ui);

    if (ui.currentView->file) [[likely]]
    {
        vertical.push_back(
            flexbox(
                {vbox(ui.currentView->ringBuffer | toFtxuiText)},
                FlexboxConfig()
                    .Set(FlexboxConfig::Wrap::NoWrap)
                    .Set(FlexboxConfig::AlignItems::FlexStart)
                    .Set(FlexboxConfig::AlignContent::FlexStart)
                    .Set(FlexboxConfig::JustifyContent::FlexStart)) | flex);
    }
    else
    {
        vertical.push_back(vbox({text("Loading...") | center}) | center | flex);
    }

    if (ui.active == UIElement::picker)
    {
        return dbox(
            vbox(std::move(vertical)) | flex,
            renderPickerWindow(ui, context)) | flex;
    }
    else
    {
        return vbox(std::move(vertical)) | flex;
    }
}

static Element renderStatusLine(Ftxui& ui, bool isCommand)
{
    const auto fileName = ui.currentView
        ? ui.currentView->file
            ? ui.currentView->file->path().c_str()
            : "loading..."
        : "[No Name]";

    return hbox({
        text(isCommand ? " COMMAND " : " NORMAL ")
            | bgcolor(isCommand ? Palette::StatusLine::commandBg : Palette::StatusLine::normalBg)
            | color(isCommand ? Palette::StatusLine::commandFg : Palette::StatusLine::normalFg) | bold,
        text("")
            | color(isCommand ? Palette::StatusLine::commandBg : Palette::StatusLine::normalBg)
            | bgcolor(Palette::StatusLine::bg2),
        text(" ")
            | color(Palette::StatusLine::bg2)
            | bgcolor(Palette::StatusLine::bg1),
        text(fileName)
            | bgcolor(Palette::StatusLine::bg1)
            | flex,
        text("")
            | color(Palette::StatusLine::bg3)
            | bgcolor(Palette::StatusLine::bg1),
        text("   ") | bgcolor(Palette::StatusLine::bg3)
    });
}

static Element renderCommandLine(Ftxui& ui)
{
    if (ui.active == UIElement::commandLine)
    {
        return hbox(text(":"), ui.commandLine.input->Render());
    }

    switch (ui.commandLine.severity)
    {
        case error:
            return color(Color::Red, text(ui.commandLine.messageLine.span().data()));
        case warning:
            return color(Color::Yellow, text(ui.commandLine.messageLine.span().data()));
        default:
            return text(ui.commandLine.messageLine.span().data());
    }
}

static Element render(Ftxui& ui, core::Context& context)
{
    const auto isCommand = ui.active == UIElement::commandLine;

    auto children = Elements{
        ui.currentView
            ? renderView(ui, context)
            : renderEmptyView(ui, context),
        renderStatusLine(ui, isCommand),
        renderCommandLine(ui)
    };

    return vbox(std::move(children)) | flex;
}

static bool handleEvent(const Event& event, Ftxui& ui, core::Context& context)
{
    static const std::flat_set<Event> notPassedToPicker = {
        Event::Escape,
        Event::Return,
        Event::Tab,
        Event::TabReverse,
    };

    logger << debug << event.DebugString();

    if (ui.active == UIElement::picker and not notPassedToPicker.contains(event))
    {
        ui.picker.content->TakeFocus(); // Make sure that picker view is always focused
        return ui.picker.input->OnEvent(event);
    }

    if (ui.active == UIElement::logView)
    {
        if (event.is_character())
        {
            auto character = event.character()[0];
            if (character == ':')
            {
                clearMessageLine(ui);
                ui.commandLine.inputLine.clear();
                ui.active = UIElement::commandLine;
                ui.commandLine.input->TakeFocus();
                return true;
            }

            if (core::registerKeyPress(event.character()[0], context))
            {
                return true;
            }
            return false;
        }
    }

    const auto handler = ui.eventHandlers.find(event);

    if (handler == ui.eventHandlers.end())
    {
        if (ui.active == UIElement::logView)
        {
            const auto& input = event.input();
            if (input.size() == 1)
            {
                if (input[0] < 0x20)
                {
                    core::registerKeyPress(input[0], context);
                    return true;
                }
            }
        }
        return false;
    }

    return handler->second(ui, context);
}

ViewNode* getActiveLineView(Ftxui& ui)
{
    auto view = ui.mainView.activeChild();

    while (view->activeChild())
    {
        if (view->depth() == ui.activeLine)
        {
            break;
        }
        view = view->activeChild();
    }

    return view;
}

static bool activeLineLeft(Ftxui& ui, core::Context&)
{
    if (ui.active != logView)
    {
        return false;
    }

    auto prev = getActiveLineView(ui)->prev();

    if (prev)
    {
        prev->setActive();
        ui.currentView = prev->parent()->deepestActive()->ptrCast<View>();
    }

    return true;
}

static bool activeLineRight(Ftxui& ui, core::Context&)
{
    if (ui.active != logView)
    {
        return false;
    }

    auto next = getActiveLineView(ui)->next();

    if (next)
    {
        next->setActive();
        ui.currentView = next->parent()->deepestActive()->ptrCast<View>();
    }

    return true;
}

static bool activeLineUp(Ftxui& ui, core::Context&)
{
    if (ui.active != logView)
    {
        return false;
    }

    ui.activeLine = (ui.activeLine - 1) | utils::clamp(0, ui.currentView->depth());
    return true;
}

static bool activeLineDown(Ftxui& ui, core::Context&)
{
    if (ui.active != logView)
    {
        return false;
    }

    ui.activeLine = (ui.activeLine + 1) | utils::clamp(0, ui.currentView->depth());
    return true;
}

Ftxui::Ftxui()
    : screen(ScreenInteractive::Fullscreen())
    , showLineNumbers(false)
    , mainView(ViewNode::Type::link, "main")
    , currentView(nullptr)
    , activeLine(0)
    , active(UIElement::logView)
    , eventHandlers{
        {Event::PageUp,         whenActive(UIElement::logView, pageUp)},
        {Event::PageDown,       whenActive(UIElement::logView, pageDown)},
        {Event::ArrowDown,      whenActive(UIElement::logView, arrowDown)},
        {Event::ArrowUp,        whenActive(UIElement::logView, arrowUp)},
        {Event::ArrowLeft,      whenActive(UIElement::logView, arrowLeft)},
        {Event::ArrowRight,     whenActive(UIElement::logView, arrowRight)},
        {Event::Escape,         escape},
        {Event::CtrlC,          ctrlC},
        {Event::CtrlZ,          ignore},
        {Event::Return,         enter},
        {Event::Resize,         resize},
        {Event::F12,            abort},
        {Event::Tab,            tab},
        {Event::TabReverse,     tabReverse},
        {Event::ArrowLeftCtrl,  activeLineLeft},
        {Event::ArrowRightCtrl, activeLineRight},
        {Event::ArrowUpCtrl,    activeLineUp},
        {Event::ArrowDownCtrl,  activeLineDown},
    }
{
}

void Ftxui::run(core::Context& context)
{
    commandLine.input = Input(
        &commandLine.inputLine,
        InputOption{
            .multiline = false
        });

    createPicker(*this, context);

    auto component
        = Renderer(
            Container::Stacked({
                picker.window,
                Container::Vertical({commandLine.input})
            }),
            [&context, this]{ return render(*this, context); })
        | CatchEvent([&context, this](const Event& event){ return handleEvent(event, *this, context); })
        | utils::sideEffect([this, &context](auto&){ resize(*this, context); });

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
    ::ui::scrollTo(*this, lineNumber, context);
}

std::ostream& Ftxui::operator<<(Severity severity)
{
    clearMessageLine(*this);
    commandLine.severity = severity;
    return commandLine.messageLine;
}

void* Ftxui::createView(std::string name, core::Context&)
{
    auto& link = mainView
        .addChild(ViewNode::createLink(std::move(name)))
        .setActive()
        .depth(0);

    currentView = link
        .addChild(View::create("base"))
        .setActive()
        .ptrCast<View>();

    return &link;
}

void Ftxui::attachFileToView(core::MappedFile& file, void* handle, core::Context& context)
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

CommandLine::CommandLine()
    : severity(info)
    , buffer{}
    , messageLine(buffer)
{
}

DEFINE_COMMAND(grep)
{
    HELP() = "filter current view";
    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "pattern")
        };
    }

    EXECUTOR()
    {
        const auto& pattern = args[0].string;
        auto& ui = context.ui->get<Ftxui>();

        if (not isViewLoaded(ui))
        {
            ui << error << "no file loaded yet";
            return false;
        }

        auto viewToAdd = ui.currentView->isBase()
            ? ui.currentView->parent()
            : ui.currentView;

        auto& newLink = (*viewToAdd)
            .addChild(ViewNode::createLink(pattern))
            .setActive();

        auto& base = newLink
            .addChild(View::create("base"))
            .setActive()
            .cast<View>();

        auto parentView = ui.currentView;
        ui.currentView = &base;

        core::asyncGrep(
            pattern,
            parentView->lines,
            *parentView->file,
            [&base, &ui, &context, file = parentView->file](core::LineRefs lines, float time)
            {
                ui.screen.Post(
                    [&ui, &base, &context, lines = std::move(lines), file, time]
                    {
                        ui << info << "found " << lines.size() << " matches; took "
                           << std::fixed << std::setprecision(3) << time << " s";

                        base.file = file;
                        base.lineCount = lines.size();
                        base.lines = std::move(lines);

                        reloadView(base, ui, context);
                    });
                ui.screen.RequestAnimationFrame();
            });

        return true;
    }
}

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return ui.showLineNumbers;
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();

        ui.showLineNumbers = value.boolean;

        if (ui.currentView and ui.currentView->file)
        {
            reloadLines(*ui.currentView, context);
        }

        return true;
    }
}

DEFINE_READONLY_VARIABLE(path, string, "Path to the opened file")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();

        if (not isViewLoaded(ui))
        {
            return nullptr;
        }

        return &ui.currentView->file->path();
    }
}

}  // namespace ui

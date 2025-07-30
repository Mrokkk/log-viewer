#include "ftxui.hpp"

#include <bit>
#include <cstdlib>
#include <cstring>
#include <flat_set>
#include <iomanip>
#include <ranges>
#include <spanstream>
#include <sstream>

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
#include "utils/math.hpp"
#include "utils/side_effect.hpp"
#include "utils/string.hpp"

using namespace ftxui;

namespace core
{

// This should be somewhere else, but I need to figure out how
// to handle string coloring in a generic manner not dependent
// on ftxui and without excessive number of string allocations
std::string getLine(core::Context& context, View& view, size_t lineIndex)
{
    std::stringstream ss;

    if (context.showLineNumbers)
    {
        ss << std::setw(view.lineNrDigits + 1) << std::right
           << ColorWrapped(lineIndex, 0x4e4e4e_rgb)
           << ColorWrapped("│ ", 0x4e4e4e_rgb);
    }

    auto line = (*view.file)[lineIndex];
    auto xoffset = view.xoffset | utils::clamp(0ul, line.length());

    if (line.find("ERR") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Red);
    }
    else if (line.find("WRN") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Yellow);
    }
    else if (line.find("DBG") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Palette256(245));
    }
    else
    {
        ss << line.c_str() + xoffset;
    }

    return ss.str();
}

void reloadView(View& view, Context& context)
{
    auto& ui = context.ui->get<ui::Ftxui>();
    view.viewHeight = std::min(static_cast<size_t>(ui.terminalSize.dimy) - 2, view.file->lineCount());
    view.lineNrDigits = utils::numberOfDigits(view.file->lineCount());
    view.ringBuffer = utils::RingBuffer<std::string>(view.viewHeight);
    view.yoffset = view.yoffset | utils::clamp(0ul, view.file->lineCount() - view.viewHeight);

    reloadLines(view, context);
}

}  // namespace core

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

static const MenuEntryOption menuOption{
    .transform =
        [](const EntryState& state)
        {
            if (state.focused)
            {
                return text("▌" + state.label) | color(Color::Blue) | bold;
            }
            else
            {
                return text(" " + state.label);
            }
        }
};

static void loadPicker(Ftxui& ui, Picker::Type type, core::Context& context)
{
    ui.picker.content->DetachAllChildren();

    const auto width = ui.terminalSize.dimx / 2 - 4;
    const auto leftWidth = width / 2;

    switch (type)
    {
        using namespace std;
        using namespace std::ranges;

        case Picker::Type::files:
            ui.picker.strings = core::readCurrentDirectoryRecursive();
            break;

        case Picker::Type::views:
            ui.picker.strings = context.views
                | views::transform([](const core::View& e){ return e.file ? e.file->path() : "loading..."; })
                | to<utils::Strings>();
            break;

        case Picker::Type::commands:
            ui.picker.strings = core::Commands::map()
                | views::transform(
                    [leftWidth, width](const auto& e)
                    {
                        std::stringstream commandDesc;
                        commandDesc << e.second.name << " ";
                        for (const auto& arg : e.second.arguments.get())
                        {
                            if (arg.type == core::Type::variadic)
                            {
                                commandDesc << '[' << arg.name << "]... ";
                            }
                            else
                            {
                                commandDesc << '[' << arg.type << ':' << arg.name << "] ";
                            }
                        }

                        std::stringstream ss;
                        ss << std::left << std::setw(leftWidth) << commandDesc.str()
                           << std::right << std::setw(width - leftWidth) << ColorWrapped(e.second.help, 0x4e4e4e_rgb);

                        return ss.str();
                    })
                | to<utils::Strings>();
            break;

        case Picker::Type::variables:
            ui.picker.strings = core::Variables::map()
                | views::transform(
                    [&context, leftWidth, width](const auto& e)
                    {
                        std::stringstream variableDesc;
                        variableDesc << e.second.name << '{';
                        if (not e.second.writer)
                        {
                            variableDesc << "const ";
                        }

                        variableDesc << e.second.type << "} : " << core::VariableWithContext{e.second, context};

                        std::stringstream ss;
                        ss << std::left << std::setw(leftWidth) << variableDesc.str()
                           << std::right << std::setw(width - leftWidth) << ColorWrapped(e.second.help, 0x4e4e4e_rgb);

                        return ss.str();
                    })
                | to<utils::Strings>();
            break;

        default:
            break;
    }

    ui.picker.cachedStrings = core::fuzzyFilter(ui.picker.strings, "");

    for (auto& entry : ui.picker.cachedStrings)
    {
        ui.picker.content->Add(MenuEntry(entry, menuOption));
    }

    if (ui.picker.cachedStrings.size())
    {
        ui.picker.content->SetActiveChild(ui.picker.content->ChildAt(0));
    }

    ui.picker.inputLine.clear();
    ui.picker.active = type;
}

static void showPicker(Ftxui& ui, Picker::Type type, core::Context& context)
{
    loadPicker(ui, type, context);

    ui.picker.content->TakeFocus();
    ui.active = UIElement::picker;
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

static void pickerAccept(Ftxui& ui, core::Context& context)
{
    auto active = ui.picker.content->ActiveChild();

    if (not active) [[unlikely]]
    {
        return;
    }

    const auto index = static_cast<size_t>(active->Index());

    if (index >= ui.picker.cachedStrings.size()) [[unlikely]]
    {
        return;
    }

    switch (ui.picker.active)
    {
        case Picker::Type::files:
            executeCode("open \"" + *ui.picker.cachedStrings[active->Index()] + '\"', context);
            break;

        case Picker::Type::views:
            context.currentView = &context.views[index];
            break;

        default:
            break;
    }

    ui.active = UIElement::logView;
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

static std::string pickerName(const Picker::Type type)
{
#define TYPE_CONVERT(type) \
    case Picker::Type::type: return #type
    switch (type)
    {
        TYPE_CONVERT(files);
        TYPE_CONVERT(views);
        TYPE_CONVERT(commands);
        TYPE_CONVERT(variables);
        case Picker::Type::_last:
            break;
    }
    return "unknown";
}

static utils::Strings getPickerNames()
{
    utils::Strings names;
    for (int i = 0; i < static_cast<int>(Picker::Type::_last); ++i)
    {
        names.push_back(pickerName(static_cast<Picker::Type>(i)));
    }
    return names;
}

static void pickerNext(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(ui.picker.active + 1);

    if (nextType == Picker::Type::_last)
    {
        nextType = Picker::Type::files;
    }

    loadPicker(ui, nextType, context);
}

static void pickerPrev(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(ui.picker.active - 1);

    if (static_cast<int>(nextType < 0))
    {
        nextType = static_cast<Picker::Type>(Picker::Type::_last - 1);
    }

    loadPicker(ui, nextType, context);
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

    if (not context.currentView or not context.currentView->file)
    {
        return false;
    }

    if (ui.terminalSize.dimy != oldTerminalSize.dimy)
    {
        logger << info << "terminal size: " << ui.terminalSize.dimx << 'x' << ui.terminalSize.dimy;

        for (auto& view : context.views)
        {
            reloadView(view, context);
        }
    }

    return true;
}

static bool pageUp(Ftxui&, core::Context& context)
{
    core::scrollPageUp(context);
    return true;
}

static bool pageDown(Ftxui&, core::Context& context)
{
    core::scrollPageDown(context);
    return true;
}

static bool arrowDown(Ftxui&, core::Context& context)
{
    core::scrollLineDown(context);
    return true;
}

static bool arrowUp(Ftxui&, core::Context& context)
{
    core::scrollLineUp(context);
    return true;
}

static bool arrowLeft(Ftxui&, core::Context& context)
{
    core::scrollLeft(context);
    return true;
}

static bool arrowRight(Ftxui&, core::Context& context)
{
    core::scrollRight(context);
    return true;
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

static Element renderPickerWindow(Ftxui& ui, core::Context&)
{
    const auto resx = ui.terminalSize.dimx / 2;
    const auto resy = ui.terminalSize.dimy / 2;

    char buffer[128];
    std::ospanstream ss(buffer);

    ss << ui.picker.cachedStrings.size() << '/' << ui.picker.strings.size();

    return window(
        text(""),
        vbox({
            ui.picker.tabs->Render(),
            separator(),
            hbox({
                ui.picker.input->Render() | xflex,
                separator(),
                text(std::string(ss.span().begin(), ss.span().end()))
            }),
            separator(),
            ui.picker.content->Render() | yframe
        }) | flex) | size(WIDTH, EQUAL, resx) | size(HEIGHT, EQUAL, resy) | clear_under | center | flex;
}

static Element renderEmptyView(core::Context& context)
{
    auto& ui = context.ui->get<Ftxui>();

    if (ui.active == UIElement::picker)
    {
        return dbox(text(""), renderPickerWindow(ui, context)) | flex;
    }

    return vbox({
        text("Hello!") | center,
        text("Press CTRL-P to open file picker") | center,
        text("Alternatively, type :e <file><Enter> to open given file") | center,
    }) | center | flex;
}

static Element renderLoadingView(core::Context& context)
{
    auto& ui = context.ui->get<Ftxui>();

    if (ui.active == UIElement::picker)
    {
        return dbox(text(""), renderPickerWindow(ui, context)) | flex;
    }

    return vbox({text("Loading...") | center}) | center | flex;
}

static Element renderView(core::View& view, core::Context& context)
{
    auto& ui = context.ui->get<Ftxui>();

    if (ui.active == UIElement::picker)
    {
        return dbox(
            flexbox(
                {vbox(view.ringBuffer | toFtxuiText)},
                FlexboxConfig()
                    .Set(FlexboxConfig::Wrap::NoWrap)
                    .Set(FlexboxConfig::AlignItems::FlexStart)
                    .Set(FlexboxConfig::AlignContent::FlexStart)
                    .Set(FlexboxConfig::JustifyContent::FlexStart)) | flex,
            renderPickerWindow(ui, context)) | flex;
    }
    else
    {
        return flexbox(
            {vbox(view.ringBuffer | toFtxuiText)},
            FlexboxConfig()
                .Set(FlexboxConfig::Wrap::NoWrap)
                .Set(FlexboxConfig::AlignItems::FlexStart)
                .Set(FlexboxConfig::AlignContent::FlexStart)
                .Set(FlexboxConfig::JustifyContent::FlexStart)) | flex;
    }
}

static Element renderStatusLine(core::Context& context, bool isCommand)
{
    const auto fileName = context.currentView
        ? context.currentView->file
            ? context.currentView->file->path().c_str()
            : "loading..."
        : "[No Name]";

    return hbox({
        // FIXME: I really need to define palette
        text(isCommand ? " COMMAND " : " NORMAL ") | bgcolor(isCommand ? 0x7daea3_rgb : 0xaf8787_rgb) | color(0x282828_rgb) | bold,
        text("") | color(isCommand ? 0x7daea3_rgb : 0xaf8787_rgb) | bgcolor(0x4e4e4e_rgb),
        text(" ") | color(0x4e4e4e_rgb) | bgcolor(0x303030_rgb),
        text(fileName) | bgcolor(0x303030_rgb) | flex,
        text("") | color(0x45403d_rgb) | bgcolor(0x303030_rgb),
        text("   ") | bgcolor(0x45403d_rgb)
    });
}

static Element renderCommandLine(core::Context& context)
{
    auto& ui = context.ui->get<Ftxui>();

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
        context.currentView
            ? context.currentView->file
                ? renderView(*context.currentView, context)
                : renderLoadingView(context)
            : renderEmptyView(context),
        renderStatusLine(context, isCommand),
        renderCommandLine(context)
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

void onFilePickerChange(Ftxui& ui, core::Context&)
{
    ui.picker.content->DetachAllChildren();

    ui.picker.cachedStrings = core::fuzzyFilter(ui.picker.strings, ui.picker.inputLine);

    for (auto& entry : ui.picker.cachedStrings)
    {
        ui.picker.content->Add(MenuEntry(entry, menuOption));
    }
}

Ftxui::Ftxui()
    : screen(ScreenInteractive::Fullscreen())
    , active(UIElement::logView)
    , eventHandlers{
        {Event::PageUp,     whenActive(UIElement::logView, pageUp)},
        {Event::PageDown,   whenActive(UIElement::logView, pageDown)},
        {Event::ArrowDown,  whenActive(UIElement::logView, arrowDown)},
        {Event::ArrowUp,    whenActive(UIElement::logView, arrowUp)},
        {Event::ArrowLeft,  whenActive(UIElement::logView, arrowLeft)},
        {Event::ArrowRight, whenActive(UIElement::logView, arrowRight)},
        {Event::Escape,     escape},
        {Event::CtrlC,      ctrlC},
        {Event::CtrlZ,      ignore},
        {Event::Return,     enter},
        {Event::Resize,     resize},
        {Event::F12,        abort},
        {Event::Tab,        tab},
        {Event::TabReverse, tabReverse},
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

    picker.content = Container::Vertical({});
    picker.input = Input(
        &picker.inputLine,
        InputOption{
            .multiline = false,
            .on_change = [this, &context]{ onFilePickerChange(*this, context); },
        }
    );

    picker.tabs = Toggle(
        getPickerNames(),
        std::bit_cast<int*>(&picker.active));

    picker.window = Window({
        .inner = Container::LockedVertical({
            picker.tabs,
            picker.input,
            picker.content
        }),
        .title = "",
        .resize_left = false,
        .resize_right = false,
        .resize_top = false,
        .resize_down = false,
    });

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

void Ftxui::execute(std::function<void()> fn)
{
    screen.Post(std::move(fn));
    screen.RequestAnimationFrame();
}

void Ftxui::executeShell(const std::string& cmd)
{
    auto command = cmd + "; read -n 1 -s -r -p \"\nCommand exited with $?; press any key to continue\"";
    screen.Post(screen.WithRestoredIO(
        [command = std::move(command)]
        {
            std::system(command.c_str());
        }));
}

std::ostream& Ftxui::operator<<(Severity severity)
{
    clearMessageLine(*this);
    commandLine.severity = severity;
    return commandLine.messageLine;
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

DEFINE_COMMAND(files)
{
    HELP() = "show file picker";
    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        showPicker(context.ui->get<Ftxui>(), Picker::Type::files, context);
        return true;
    }
}

DEFINE_COMMAND(views)
{
    HELP() = "show variables picker";
    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        showPicker(context.ui->get<Ftxui>(), Picker::Type::views, context);
        return true;
    }
}

DEFINE_COMMAND(commands)
{
    HELP() = "show commands picker";
    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        showPicker(context.ui->get<Ftxui>(), Picker::Type::commands, context);
        return true;
    }
}

DEFINE_COMMAND(variables)
{
    HELP() = "show variables picker";
    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        showPicker(context.ui->get<Ftxui>(), Picker::Type::variables, context);
        return true;
    }
}

}  // namespace ui

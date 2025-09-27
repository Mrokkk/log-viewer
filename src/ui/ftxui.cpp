#define LOG_HEADER "ui::Ftxui"
#include "ftxui.hpp"

#include <cstdlib>
#include <expected>
#include <memory>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string_internal.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/command_line.hpp"
#include "core/context.hpp"
#include "core/event.hpp"
#include "core/events/key_press.hpp"
#include "core/events/resize.hpp"
#include "core/grepper.hpp"
#include "core/input.hpp"
#include "core/main_picker.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "core/severity.hpp"
#include "core/thread.hpp"
#include "sys/system.hpp"
#include "ui/palette.hpp"
#include "ui/window_renderer.hpp"
#include "utils/math.hpp"

using namespace ftxui;

template<>
struct std::hash<ftxui::Event>
{
    std::size_t operator()(const ftxui::Event& e) const noexcept
    {
        return std::hash<std::string>{}(e.input());
    }
};

namespace ui
{

static utils::HashMap<Event, core::KeyPress> eventToKeyPress;

static utils::Maybe<core::KeyPress> convertEvent(const ftxui::Event& event)
{
    auto node = eventToKeyPress.find(event);

    if (not node) [[unlikely]]
    {
        return {};
    }

    return node->second;
}

static void initEventConverter()
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
        {Event::Character(' '), core::KeyPress::space},
        {Event::Special({'\e', '[', '1', '~'}), core::KeyPress::home},
        {Event::Special({'\e', '[', '4', '~'}), core::KeyPress::end},
        {Event::Special({'\e', '[', '1', ';', '2', 'A'}), core::KeyPress::shiftArrowUp},
        {Event::Special({'\e', '[', '1', ';', '2', 'B'}), core::KeyPress::shiftArrowDown},
        {Event::Special({'\e', '[', '1', ';', '2', 'D'}), core::KeyPress::shiftArrowLeft},
        {Event::Special({'\e', '[', '1', ';', '2', 'C'}), core::KeyPress::shiftArrowRight},
    };

    for (char c = '@'; c <= 'Z'; ++c)
    {
        char small = c + 0x20;
        char capital = c;
        char ctrl = c - 0x40;

        // Small letter
        eventToKeyPress.insert(
            Event::Character(std::string{small}),
            core::KeyPress::character(small));

        // Capital letter
        eventToKeyPress.insert(
            Event::Character(std::string{capital}),
            core::KeyPress::character(capital));

        auto ctrlCharacter = Event::Special(std::string{ctrl});

        if (not eventToKeyPress.find(ctrlCharacter))
        {
            // Ctrl+letter
            eventToKeyPress.insert(
                std::move(ctrlCharacter),
                core::KeyPress::ctrl(small));
        }

        // Alt+letter
        eventToKeyPress.insert(
            Event::Special(std::string{'\e', small}),
            core::KeyPress::alt(small));
    }

    for (char c = '!'; c <= '?'; ++c)
    {
        eventToKeyPress.insert(
            Event::Character(std::string{c}),
            core::KeyPress::character(c));
    }
}

static bool abort(Ftxui&, core::Context&)
{
    std::abort();
    return true;
}

static bool resize(Ftxui& ui, core::Context& context)
{
    ui.terminalSize = Terminal::Size();
    core::sendEvent<core::events::Resize>(core::InputSource::user, context, ui.terminalSize.dimx, ui.terminalSize.dimy);
    return true;
}

static Element renderHelp(Ftxui& ui, core::Context& context)
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

    return vbox(
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
            ));
}

static Element renderStatusLine(Ftxui&, core::Context& context)
{
    const auto fileName{context.mainView.activeFileName()};
    Color statusFg, statusBg;
    const char* status;

    switch (context.mode)
    {
        case core::Mode::command:
            statusFg = Palette::StatusLine::commandFg;
            statusBg = Palette::StatusLine::commandBg;
            status = " COMMAND ";
            break;

        case core::Mode::visual:
            statusFg = Palette::StatusLine::visualFg;
            statusBg = Palette::StatusLine::visualBg;
            status = " VISUAL ";
            break;

        case core::Mode::normal:
            statusFg = Palette::StatusLine::normalFg;
            statusBg = Palette::StatusLine::normalBg;
            status = " NORMAL ";
            break;

        case core::Mode::picker:
            statusFg = Palette::StatusLine::normalFg;
            statusBg = Palette::StatusLine::normalBg;
            status = " PICKER ";
            break;

        case core::Mode::grepper:
            statusFg = Palette::StatusLine::normalFg;
            statusBg = Palette::StatusLine::normalBg;
            status = " GREPPER ";
            break;

        [[unlikely]] default:
            statusFg = Palette::StatusLine::normalFg;
            statusBg = Palette::StatusLine::normalBg;
            status = " UNEXPECTED ";
            break;
    }

    utils::Buffer buf;

    if (const auto node = context.mainView.currentWindowNode()) [[likely]]
    {
        const auto& w = node->window();
        const auto lineCount = w.lineCount;
        const auto currentLine = w.ycurrent + w.yoffset + 1;
        const auto currentPosition = w.xcurrent + w.xoffset + 1;
        buf << " " << currentLine << '/' << lineCount << " ℅:" << currentPosition << ' ';
    }

    return hbox({
        text(status)
            | color(statusFg)
            | bgcolor(statusBg)
            | bold,
        text("")
            | color(statusBg)
            | bgcolor(Palette::StatusLine::bg2),
        text(" ")
            | color(Palette::StatusLine::bg2)
            | bgcolor(Palette::StatusLine::bg1),
        text(fileName)
            | bgcolor(Palette::StatusLine::bg1)
            | flex,
        text("")
            | color(statusBg)
            | bgcolor(Palette::StatusLine::bg1),
        text(" ")
            | bgcolor(statusBg),
        text(buf.str())
            | color(statusFg)
            | bgcolor(statusBg)
            | bold,
        text(core::inputStateString(context))
            | color(statusFg)
            | bgcolor(statusBg),
        text(" ")
            | bgcolor(statusBg),
    });
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
            overlays.emplace_back(renderHelp(ui, context));
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
        core::sendEvent<core::events::KeyPress>(core::InputSource::user, context, *keyPress);
        return true;
    }

    if (event == Event::Resize)
    {
        resize(ui, context);
        return true;
    }
    else if (event == Event::F12)
    {
        abort(ui, context);
        return true;
    }

    return false;
}

static inline ScreenInteractive createScreen()
{
    return ScreenInteractive::Fullscreen();
}

Ftxui::Ftxui(core::Context& context)
    : screen(createScreen())
    , commandLine(context)
    , picker(context)
    , grepper(context)
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

Element MainView::render(core::Context& context)
{
    auto current = context.mainView.currentWindowNode();

    if (not current) [[unlikely]]
    {
        return vbox({
            text("Hello!") | center,
            text("Type :files<Enter> to open file picker") | center,
            text("Alternatively, type :e <file><Enter> to open given file") | center,
        }) | center | flex;
    }

    auto vertical = renderTablines(context);

    if (current->loaded()) [[likely]]
    {
        vertical.push_back(std::make_shared<WindowRenderer>(current->window()));
    }
    else
    {
        vertical.push_back(vbox({text("Loading...") | center}) | center | flex);
    }

    return vbox(std::move(vertical)) | flex;
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

static Element renderTab(const std::string& label, int index, bool active)
{
    auto string = ' ' + std::to_string(index) + ' ' + label + ' ';

    if (active)
    {
        if (index != 0)
        {
            return hbox({
                text("")
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg),
                text(std::move(string))
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg)
                    | bold,
                text("")
                    | color(Palette::TabLine::activeBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
        else
        {
            return hbox({
                text(std::move(string))
                    | bgcolor(Palette::TabLine::activeBg)
                    | color(Palette::TabLine::activeFg)
                    | bold,
                text("")
                    | color(Palette::TabLine::activeBg)
                    | bgcolor(Palette::TabLine::activeFg),
            });
        }
    }
    else
    {
        if (index != 0)
        {
            return hbox({
                text("")
                    | bgcolor(Palette::TabLine::inactiveBg)
                    | color(Palette::TabLine::separatorBg),
                text(std::move(string))
                    | bgcolor(Palette::TabLine::inactiveBg),
                text("")
                    | color(Palette::TabLine::inactiveBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
        else
        {
            return hbox({
                text(std::move(string))
                    | bgcolor(Palette::TabLine::inactiveBg),
                text("")
                    | color(Palette::TabLine::inactiveBg)
                    | bgcolor(Palette::TabLine::separatorBg),
            });
        }
    }
}

Elements MainView::renderTablines(core::Context& context)
{
    Elements vertical;
    vertical.reserve(10);

    const auto& root = context.mainView.root();

    Elements horizontal;
    horizontal.reserve(std::max(root.children().size(), 10uz));

    int i = 0;
    int index = 0;
    int activeTabline = context.mainView.activeTabline();

    for (const auto& child : root.children())
    {
        horizontal.push_back(renderTab(child->name(), i, root.activeChild() == child));
        ++i;
    }

    vertical.push_back(wrapActiveLineIf(hbox(std::move(horizontal)), index++ == activeTabline));

    auto node = root.activeChild();

    if (not node) [[unlikely]]
    {
        return vertical;
    }

    while (node->activeChild())
    {
        horizontal.reserve(10);
        int i = 0;
        for (const auto& child : node->children())
        {
            horizontal.push_back(renderTab(child->name(), i, child == node->activeChild()));
            ++i;
        }

        vertical.push_back(wrapActiveLineIf(hbox(std::move(horizontal)), index++ == activeTabline));

        node = node->activeChild();
    }

    return vertical;
}

CommandLine::CommandLine(core::Context& context)
    : mCommandTextBox({
        .content = context.commandLine.commandReadline.line(),
        .cursorPosition = &context.commandLine.commandReadline.cursor(),
        .suggestion = &context.commandLine.commandReadline.suggestion(),
        .suggestionColor = &Palette::bg5
    })
    , mSearchTextBox({
        .content = context.commandLine.searchReadline.line(),
        .cursorPosition = &context.commandLine.searchReadline.cursor(),
    })
{
}

static ftxui::Element renderPickerEntry(const std::string& entry, bool active)
{
    if (active)
    {
        return ftxui::text("▌" + entry) | color(Palette::Picker::activeLineMarker) | ftxui::bold;
    }
    else
    {
        return ftxui::text(" " + entry);
    }
}

static Element renderPicker(const core::Picker& picker)
{
    const auto& pickerContent = picker.filtered();
    Elements elements;

    const size_t resy = picker.height();

    size_t pickerSize = utils::clamp(pickerContent.size(), 0uz, resy);
    elements.reserve(pickerSize);

    for (size_t i = 0; i < resy - pickerSize; ++i)
    {
        elements.emplace_back(filler());
    }

    size_t i = 0;
    size_t cursor = picker.cursor();

    for (const auto& entry : pickerContent)
    {
        const bool active = i == cursor;
        auto element = renderPickerEntry(*entry, active);
        if (active) element |= focus;
        elements.emplace_back(std::move(element));
        ++i;
    }

    return vbox(elements)
        | yframe
        | size(HEIGHT, EQUAL, resy);
}

static Element renderCompletions(const utils::StringViews& completions, core::Context& context)
{
    Elements elements{
        text(" COMPLETION ")
            | color(Palette::StatusLine::commandFg)
            | bgcolor(Palette::StatusLine::commandBg)
            | bold,
        text("")
            | color(Palette::StatusLine::commandBg)
            | bgcolor(Palette::StatusLine::bg2),
        text(" ")
            | color(Palette::StatusLine::bg2)
    };

    for (auto it = completions.begin(); it != completions.end(); ++it)
    {
        if (it == context.commandLine->currentCompletion())
        {
            elements.push_back(text(std::string(*it)) | inverted);
        }
        else
        {
            elements.push_back(text(std::string(*it)));
        }
        elements.push_back(separatorCharacter(" "));
    }

    return hbox(std::move(elements));
}

static Element renderTextBox(const TextBox& options)
{
    if (not options.cursorPosition)
    {
        return text(&options.content);
    }

    if (options.content.empty())
    {
        return hbox(text(" ") | inverted);
    }

    if (*options.cursorPosition >= options.content.size())
    {
        if (options.suggestion and not options.suggestion->empty())
        {
            return hbox({
                text(&options.content),
                text(std::string{options.suggestion->at(0)}) | inverted,
                text(
                    options.suggestion->substr(1))
                        | color(options.suggestionColor
                            ? *options.suggestionColor
                            : Color()),
            });
        }
        else
        {
            return hbox({
                text(&options.content),
                text(" ") | inverted,
            });
        }
    }
    else
    {
        const int glyphStart = *options.cursorPosition;
        const int glyphEnd = static_cast<int>(GlyphNext(options.content, glyphStart));
        const auto partBeforeCursor = options.content.substr(0, glyphStart);
        const auto partAtCursor = options.content.substr(glyphStart, glyphEnd - glyphStart);
        const auto partAfterCursor = options.content.substr(glyphEnd);

        Elements e = {
            text(partBeforeCursor),
            text(partAtCursor) | inverted,
            text(partAfterCursor)
        };

        if (options.suggestion and not options.suggestion->empty())
        {
            e.push_back(
                text(*options.suggestion)
                    | color(options.suggestionColor
                        ? *options.suggestionColor
                        : Color()));
        }

        return hbox(std::move(e));
    }
}

Element CommandLine::render(core::Context& context)
{
    if (context.mode == core::Mode::command)
    {
        auto& completions = context.commandLine->completions();

        TextBox* textBox = nullptr;

        std::string commandLinePrefix{char(context.commandLine.mode())};;

        switch (context.commandLine.mode())
        {
            case core::CommandLine::Mode::command:
                textBox = &mCommandTextBox;
                break;
            case core::CommandLine::Mode::searchForward:
            case core::CommandLine::Mode::searchBackward:
                textBox = &mSearchTextBox;
                break;
        }

        auto picker = context.commandLine->picker();

        if (picker)
        {
            return vbox(
                renderPicker(*picker),
                hbox(
                    text(" FUZZY ")
                        | color(Palette::StatusLine::commandFg)
                        | bgcolor(Palette::StatusLine::commandBg)
                        | bold,
                    text("")
                        | color(Palette::StatusLine::commandBg)
                        | bgcolor(Palette::StatusLine::bg2),
                    text(" ")
                        | color(Palette::StatusLine::bg2)),
                hbox(
                    text(std::move(commandLinePrefix)),
                    renderTextBox(mCommandTextBox)));
        }
        else if (completions.empty())
        {
            return hbox(
                text(std::move(commandLinePrefix)),
                renderTextBox(*textBox));
        }

        return vbox(
                renderCompletions(completions, context),
                hbox(
                    text(std::move(commandLinePrefix)),
                    renderTextBox(*textBox)));
    }

    switch (context.messageLine.severity())
    {
        case Severity::error:
            return color(Color::Red, text(context.messageLine.str()));
        case Severity::warning:
            return color(Color::Yellow, text(context.messageLine.str()));
        default:
            return text(context.messageLine.str());
    }
}

constexpr static std::string pickerName(const core::MainPicker::Type type)
{
#define TYPE_CONVERT(type) \
    case core::MainPicker::Type::type: return #type
    switch (type)
    {
        TYPE_CONVERT(files);
        TYPE_CONVERT(commands);
        TYPE_CONVERT(variables);
        TYPE_CONVERT(messages);
        TYPE_CONVERT(logs);
        case core::MainPicker::Type::_last:
            break;
    }
    return "unknown";
}

constexpr static utils::Strings getPickerNames()
{
    utils::Strings names;
    for (int i = 0; i < static_cast<int>(core::MainPicker::Type::_last); ++i)
    {
        names.push_back(pickerName(static_cast<core::MainPicker::Type>(i)));
    }
    return names;
}

static auto pickerNames = getPickerNames();

MainPicker::MainPicker(core::Context& context)
    : mTextBox({
        .content = context.mainPicker.readline().line(),
        .cursorPosition = &context.mainPicker.readline().cursor(),
    })
{
}

static Element renderTabs(core::Context& context)
{
    Elements elements;
    elements.reserve(2 * static_cast<int>(core::MainPicker::Type::_last) - 1);
    int index = 0;
    auto activePickerIndex = context.mainPicker.currentPickerIndex();
    for (const auto& name : pickerNames)
    {
        elements.emplace_back(renderTab(name, index, index == activePickerIndex));
        ++index;
    }

    return hbox(
        hbox(std::move(elements)),
        text("")
            | color(Palette::TabLine::separatorBg)
            | bgcolor(Palette::TabLine::activeLineBg)
            | xflex);
}

Element MainPicker::MainPicker::render(core::Context& context)
{
    auto& mainPicker = context.mainPicker;
    auto& picker = mainPicker.currentPicker();

    auto& ui = context.ui->cast<Ftxui>();
    const auto resx = ui.terminalSize.dimx / 2;

    utils::Buffer buf;

    buf << picker.filtered().size() << '/' << picker.data().size();

    Elements content;
    content.reserve(picker.filtered().size());

    size_t i = 0;
    for (const auto& string : picker.filtered())
    {
        const bool active = i++ == picker.cursor();
        auto element = renderPickerEntry(*string, active);
        if (active) element |= focus;
        content.emplace_back(std::move(element));
    }

    const auto& frameColor = Palette::fg0;

    return vbox(
        renderTabs(context),
        separator() | color(frameColor),
        hbox(
            renderTextBox(mTextBox) | xflex,
            separator() | color(frameColor),
            text(buf.str())
        ),
        separator() | color(frameColor),
        vbox(std::move(content))
            | yframe
            | yflex
            | size(HEIGHT, EQUAL, picker.height()))
        | borderStyled(LIGHT, frameColor)
        | size(WIDTH, EQUAL, resx)
        | clear_under
        | center
        | flex;
}

Grepper::Grepper(core::Context& context)
    : mTextBox({
        .content = context.grepper.readline.line(),
        .cursorPosition = &context.grepper.readline.cursor(),
        .suggestion = &context.grepper.readline.suggestion(),
        .suggestionColor = &Palette::bg5
    })
{
}

Element Grepper::render(core::Context& context)
{
    auto& options = context.grepper.options;

    return window(
        text(""),
        vbox(
            renderTextBox(mTextBox),
            separator(),
            renderCheckbox(options.regex, "regex (a-r)"),
            renderCheckbox(options.caseInsensitive, "case insensitive (a-c)"),
            renderCheckbox(options.inverted, "inverted (a+i)"))
                | xflex,
        LIGHT)
            | clear_under
            | center
            | xflex;
}

Element Grepper::renderCheckbox(bool value, std::string_view description)
{
    utils::Buffer buf;
    if (value)
    {
        buf << "▣";
    }
    else
    {
        buf << "☐";
    }
    buf << ' ' << description;
    return text(buf.str());
}

void createFtxuiUserInterface(core::Context& context)
{
    auto ui = new Ftxui(context);
    context.ui = ui;
    context.mainLoop = ui;
}

}  // namespace ui

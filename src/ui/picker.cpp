#include "picker.hpp"

#include <ranges>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/colored_string.hpp>

#include "core/commands/open.hpp"
#include "core/dirs.hpp"
#include "core/fuzzy.hpp"
#include "core/input.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/symbols_map.hpp"
#include "core/logger.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "ui/event_handler.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/picker_entry.hpp"
#include "ui/ui_component.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

using namespace ftxui;

namespace ui
{

struct Picker::Impl final
{
    Impl();
    Element render(core::Context& context);
    void onExit();
    void show(Ftxui& ui, Picker::Type type, core::Context& context);
    void load(Ftxui& ui, Type type, core::Context& context);
    void prev(Ftxui& ui, core::Context& context);
    void next(Ftxui& ui, core::Context& context);
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context);

    utils::Strings    strings;
    utils::StringRefs cachedStrings;
    std::string       inputLine;
    Type              active;
    ftxui::Component  input;
    ftxui::Component  content;
    ftxui::Component  tabs;
    ftxui::Component  window;
    EventHandlers     eventHandlers;

private:
    void onFilePickerChange();
    void escape(core::Context& context);
    void accept(Ftxui& ui, core::Context& context);
};

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
        TYPE_CONVERT(messages);
        TYPE_CONVERT(logs);
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

Picker::Impl::Impl()
    : input(
        Input(
            &inputLine,
            InputOption{
                .multiline = false,
                .on_change = [this]{ onFilePickerChange(); },
            }))
    , content(Container::Vertical({}))
    , tabs(
        Toggle(
            getPickerNames(),
            std::bit_cast<int*>(&active)))
    , window(
        Window({
            .inner = Container::LockedVertical({
                tabs,
                input,
                content
            }),
            .title = "",
            .resize_left = false,
            .resize_right = false,
            .resize_top = false,
            .resize_down = false,
        }))
    , eventHandlers{
        {Event::Escape,     [this](auto&, auto& context){ escape(context); return true; }},
        {Event::Return,     [this](auto& ui, auto& context){ accept(ui, context); return true; }},
        {Event::Tab,        [this](auto& ui, auto& context){ next(ui, context); return true; }},
        {Event::TabReverse, [this](auto& ui, auto& context){ prev(ui, context); return true; }},
    }
{
}

Element Picker::Impl::render(core::Context& context)
{
    auto& ui = context.ui->cast<Ftxui>();
    const auto resx = ui.terminalSize.dimx / 2;
    const auto resy = ui.terminalSize.dimy / 2;

    utils::Buffer buf;

    buf << cachedStrings.size() << '/' << strings.size();

    return ::ftxui::window(
        text(""),
        vbox({
            tabs->Render(),
            separator(),
            hbox({
                input->Render() | xflex,
                separator(),
                text(buf.str())
            }),
            separator(),
            content->Render() | yframe
        }) | flex)
            | size(WIDTH, EQUAL, resx)
            | size(HEIGHT, EQUAL, resy)
            | clear_under
            | center
            | flex;
}

void Picker::Impl::onExit()
{
    cachedStrings.clear();
    strings.clear();
}

void Picker::Impl::show(Ftxui& ui, Picker::Type type, core::Context& context)
{
    load(ui, type, context);
    content->TakeFocus();
    switchFocus(UIComponent::picker, ui, context);
}

static const MenuEntryOption menuOption{
    .transform =
        [](const EntryState& state)
        {
            return renderPickerEntry(state.label, state.focused);
        }
};

static const Color severityToColor[4] = {
    Palette::bg5,
    Color(),
    Color::Yellow,
    Color::Red,
};

void Picker::Impl::load(Ftxui& ui, Picker::Type type, core::Context& context)
{
    content->DetachAllChildren();

    const auto width = ui.terminalSize.dimx / 2 - 4;
    const auto leftWidth = width / 2;

    switch (type)
    {
        using namespace std;
        using namespace std::ranges;

        case Picker::Type::files:
            strings = core::readCurrentDirectoryRecursive();
            break;

        case Picker::Type::views:
            strings = context.mainView.root().children()
                | views::transform([](const core::WindowNodePtr& e){ return e->name(); })
                | to<utils::Strings>();
            break;

        case Picker::Type::commands:
            strings.clear();
            core::interpreter::Commands::forEach(
                [this, leftWidth, width](const core::interpreter::Command& command)
                {
                    utils::Buffer commandDesc;

                    commandDesc << command.name << " ";

                    for (const auto& flag : command.flags.get())
                    {
                        commandDesc << "[-" << flag.name << "] ";
                    }

                    for (const auto& arg : command.arguments.get())
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

                    std::ostringstream ss;
                    ss << std::left << std::setw(leftWidth) << commandDesc.str()
                       << std::right << std::setw(width - leftWidth)
                       << ColorWrapped(command.help, Palette::Picker::additionalInfoFg);

                    strings.emplace_back(ss.str());
                });
            break;

        case Picker::Type::variables:
            strings = core::interpreter::symbolsMap()
                | views::transform(
                    [leftWidth, width](const core::interpreter::SymbolsMap::const_reference& e)
                    {
                        auto& variable = *e.second;
                        auto& variableName = e.first;

                        utils::Buffer variableDesc;
                        variableDesc << variableName << '{';

                        variableDesc << variable.value().type() << "} : " << variable;

                        std::ostringstream ss;
                        ss << std::left << std::setw(leftWidth) << variableDesc.view();

                        if (auto help = variable.help())
                        {
                           ss << std::right << std::setw(width - leftWidth)
                              << ColorWrapped(help, Palette::Picker::additionalInfoFg);
                        }

                        return ss.str();
                    })
                | to<utils::Strings>();
            break;

        case Picker::Type::messages:
            strings = context.messageLine.history();
            break;

        case Picker::Type::logs:
            strings.clear();
            logger.forEachLogEntry(
                [this](const LogEntry& entry)
                {
                    std::ostringstream ss;
                    ss << ColorWrapped(std::put_time(std::localtime(&entry.time), "%F %T"), Color::Green);
                    if (entry.header)
                    {
                        ss << ' ' << ColorWrapped(entry.header, Color::Blue) << ":";
                    }
                    ss << ' ' << ColorWrapped(entry.message, severityToColor[static_cast<int>(entry.severity)]);
                    strings.emplace_back(ss.str());
                });
            break;

        default:
            break;
    }

    cachedStrings = core::fuzzyFilter(strings, "").value_or({});

    for (auto& entry : cachedStrings)
    {
        content->Add(MenuEntry(entry, menuOption));
    }

    if (cachedStrings.size())
    {
        content->SetActiveChild(content->ChildAt(0));
    }

    inputLine.clear();
    active = type;
}

void Picker::Impl::next(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(active + 1);

    if (nextType == Picker::Type::_last)
    {
        nextType = Picker::Type::files;
    }

    load(ui, nextType, context);
}

void Picker::Impl::prev(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(active - 1);

    if (static_cast<int>(nextType < 0))
    {
        nextType = static_cast<Picker::Type>(Picker::Type::_last - 1);
    }

    load(ui, nextType, context);
}

bool Picker::Impl::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    auto result = eventHandlers.handleEvent(event, ui, context);
    if (not result)
    {
        content->TakeFocus(); // Make sure that picker view is always focused
        return input->OnEvent(event);
    }
    return *result;
}

void Picker::Impl::onFilePickerChange()
{
    content->DetachAllChildren();

    auto result = core::fuzzyFilter(strings, inputLine);

    if (not result) [[unlikely]]
    {
        cachedStrings.clear();
        return;
    }

    cachedStrings = *result;

    for (auto& entry : cachedStrings)
    {
        content->Add(MenuEntry(entry, menuOption));
    }
}

void Picker::Impl::accept(Ftxui&, core::Context& context)
{
    auto active = content->ActiveChild();

    if (not active) [[unlikely]]
    {
        return;
    }

    const auto index = static_cast<size_t>(active->Index());

    if (index >= cachedStrings.size()) [[unlikely]]
    {
        return;
    }

    switch (this->active)
    {
        case Picker::Type::files:
            core::commands::open(*cachedStrings[active->Index()], context);
            break;

        case Picker::Type::commands:
        {
            auto string = *cachedStrings[active->Index()];
            core::registerKeyPress(core::KeyPress::character(':'), core::InputSource::internal, context);
            for (auto c : string)
            {
                core::registerKeyPress(core::KeyPress::character(c), core::InputSource::internal, context);
                if (c == ' ')
                {
                    break;
                }
            }
            break;
        }

        //case Picker::Type::views:
        //{
            //ui.mainView.currentView = ui.mainView.root.childAt(index)
                //->setActive()
                //.deepestActive()
                //->ptrCast<View>();
            //break;
        //}

        default:
            break;
    }

    if (context.mode == core::Mode::custom)
    {
        switchMode(core::Mode::normal, context);
    }
}

void Picker::Impl::escape(core::Context& context)
{
    switchMode(core::Mode::normal, context);
}

Picker::Picker()
    : UIComponent(UIComponent::picker)
    , mPimpl(new Impl)
{
}

Picker::~Picker()
{
    delete mPimpl;
}

void Picker::onExit()
{
    mPimpl->onExit();
}

void Picker::takeFocus()
{
    mPimpl->content->TakeFocus();
}

ftxui::Element Picker::render(core::Context& context)
{
    return mPimpl->render(context);
}

bool Picker::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    return mPimpl->handleEvent(event, ui, context);
}

void Picker::show(Ftxui& ui, Picker::Type type, core::Context& context)
{
    mPimpl->show(ui, type, context);
}

Picker::operator ftxui::Component&()
{
    return mPimpl->window;
}

DEFINE_COMMAND(files)
{
    HELP() = "show file picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->cast<Ftxui>();
        ui.picker.show(ui, Picker::Type::files, context);
        return true;
    }
}

DEFINE_COMMAND(views)
{
    HELP() = "show variables picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->cast<Ftxui>();
        ui.picker.show(ui, Picker::Type::views, context);
        return true;
    }
}

DEFINE_COMMAND(commands)
{
    HELP() = "show commands picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->cast<Ftxui>();
        ui.picker.show(ui, Picker::Type::commands, context);
        return true;
    }
}

DEFINE_COMMAND(variables)
{
    HELP() = "show variables picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->cast<Ftxui>();
        ui.picker.show(ui, Picker::Type::variables, context);
        return true;
    }
}

DEFINE_COMMAND(messages)
{
    HELP() = "show messages picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->cast<Ftxui>();
        ui.picker.show(ui, Picker::Type::messages, context);
        return true;
    }
}

}  // namespace ui

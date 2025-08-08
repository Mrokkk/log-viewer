#include "picker.hpp"

#include <ranges>

#include <ftxui/screen/colored_string.hpp>

#include "core/command.hpp"
#include "core/dirs.hpp"
#include "core/fuzzy.hpp"
#include "core/interpreter.hpp"
#include "core/variable.hpp"
#include "ui/palette.hpp"
#include "utils/string.hpp"

using namespace ftxui;

namespace ui
{

static const MenuEntryOption menuOption{
    .transform =
        [](const EntryState& state)
        {
            if (state.focused)
            {
                return text("▌" + state.label) | color(Palette::Picker::activeLineMarker) | bold;
            }
            else
            {
                return text(" " + state.label);
            }
        }
};

void loadPicker(Ftxui& ui, Picker::Type type, core::Context& context)
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
            ui.picker.strings = ui.mainView.children()
                | views::transform([](const ViewNodePtr& e){ return e->name(); })
                | to<utils::Strings>();
            break;

        case Picker::Type::commands:
            ui.picker.strings = core::Commands::map()
                | views::transform(
                    [leftWidth, width](const auto& e)
                    {
                        std::stringstream commandDesc;

                        commandDesc << e.second.name << " ";

                        for (const auto& flag : e.second.flags.get())
                        {
                            commandDesc << "[-" << flag << "] ";
                        }

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
                           << std::right << std::setw(width - leftWidth)
                           << ColorWrapped(e.second.help, Palette::Picker::additionalInfoFg);

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
                           << std::right << std::setw(width - leftWidth)
                           << ColorWrapped(e.second.help, Palette::Picker::additionalInfoFg);

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

void showPicker(Ftxui& ui, Picker::Type type, core::Context& context)
{
    loadPicker(ui, type, context);

    ui.picker.content->TakeFocus();
    ui.active = UIElement::picker;
}

void pickerAccept(Ftxui& ui, core::Context& context)
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
        {
            executeCode("open \"" + *ui.picker.cachedStrings[active->Index()] + '\"', context);
            break;
        }

        case Picker::Type::views:
        {
            ui.currentView = ui.mainView.childAt(index)
                ->setActive()
                .deepestActive()
                ->ptrCast<View>();
            break;
        }

        default:
            break;
    }

    ui.active = UIElement::logView;
}

void pickerNext(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(ui.picker.active + 1);

    if (nextType == Picker::Type::_last)
    {
        nextType = Picker::Type::files;
    }

    loadPicker(ui, nextType, context);
}

void pickerPrev(Ftxui& ui, core::Context& context)
{
    auto nextType = static_cast<Picker::Type>(ui.picker.active - 1);

    if (static_cast<int>(nextType < 0))
    {
        nextType = static_cast<Picker::Type>(Picker::Type::_last - 1);
    }

    loadPicker(ui, nextType, context);
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

static void onFilePickerChange(Ftxui& ui, core::Context&)
{
    ui.picker.content->DetachAllChildren();

    ui.picker.cachedStrings = core::fuzzyFilter(ui.picker.strings, ui.picker.inputLine);

    for (auto& entry : ui.picker.cachedStrings)
    {
        ui.picker.content->Add(MenuEntry(entry, menuOption));
    }
}

void createPicker(Ftxui& ui, core::Context& context)
{
    ui.picker.content = Container::Vertical({});
    ui.picker.input = Input(
        &ui.picker.inputLine,
        InputOption{
            .multiline = false,
            .on_change = [&ui, &context]{ onFilePickerChange(ui, context); },
        }
    );

    ui.picker.tabs = Toggle(
        getPickerNames(),
        std::bit_cast<int*>(&ui.picker.active));

    ui.picker.window = Window({
        .inner = Container::LockedVertical({
            ui.picker.tabs,
            ui.picker.input,
            ui.picker.content
        }),
        .title = "",
        .resize_left = false,
        .resize_right = false,
        .resize_top = false,
        .resize_down = false,
    });
}

Element renderPickerWindow(Ftxui& ui, core::Context&)
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
        showPicker(context.ui->get<Ftxui>(), Picker::Type::files, context);
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
        showPicker(context.ui->get<Ftxui>(), Picker::Type::views, context);
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
        showPicker(context.ui->get<Ftxui>(), Picker::Type::commands, context);
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
        showPicker(context.ui->get<Ftxui>(), Picker::Type::variables, context);
        return true;
    }
}

}  // namespace ui

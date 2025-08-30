#include "command_line.hpp"

#include <ranges>
#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/command_line.hpp"
#include "core/message_line.hpp"
#include "core/severity.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/picker_entry.hpp"
#include "ui/text_box.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

static size_t pickerHeight(size_t terminalHeight)
{
    return terminalHeight / 3;
}

CommandLine::CommandLine(Ftxui& ui, core::Context& context)
    : UIComponent(UIComponent::commandLine)
    , mTextBox({
        .content = context.commandLine->line(),
        .cursorPosition = &context.commandLine->cursor(),
        .suggestion = &context.commandLine->suggestion(),
        .suggestionColor = &Palette::bg5
    })
{
    context.commandLine->setPageSize(pickerHeight(ui.terminalSize.dimy));
}

CommandLine::~CommandLine() = default;

void CommandLine::takeFocus()
{
}

bool CommandLine::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    if (event == Event::Resize)
    {
        context.commandLine->setPageSize(pickerHeight(ui.terminalSize.dimy));
        return true;
    }

    return false;
}

static Element renderPicker(const core::Picker& picker, Ftxui& ui)
{
    const auto& pickerContent = picker.filtered();
    Elements elements;

    const size_t resy = ui.terminalSize.dimy / 3;

    size_t pickerSize = pickerContent.size() | utils::clamp(0ul, resy);
    elements.reserve(pickerSize);

    for (size_t i = 0; i < resy - pickerSize; ++i)
    {
        elements.emplace_back(filler());
    }

    size_t i = pickerContent.size() - 1;
    size_t cursor = picker.cursor();

    for (const auto& entry : pickerContent | std::ranges::views::reverse)
    {
        const bool active = i == cursor;
        auto element = renderPickerEntry(*entry, active);
        if (active) element |= focus;
        elements.emplace_back(std::move(element));
        --i;
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

Element CommandLine::render(core::Context& context)
{
    auto& ui = context.ui->get<Ftxui>();

    if (ui.active->type == UIComponent::commandLine)
    {
        auto& completions = context.commandLine->completions();

        auto picker = context.commandLine->picker();

        if (picker)
        {
            return vbox(
                renderPicker(*picker, ui),
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
                hbox(text(":"), renderTextBox(mTextBox)));
        }
        else if (completions.empty())
        {
            return hbox(text(":"), renderTextBox(mTextBox));
        }

        return vbox(
                renderCompletions(completions, context),
                hbox(text(":"), renderTextBox(mTextBox)));
    }

    switch (context.messageLine.severity())
    {
        case error:
            return color(Color::Red, text(context.messageLine.get()));
        case warning:
            return color(Color::Yellow, text(context.messageLine.get()));
        default:
            return text(context.messageLine.get());
    }
}

}  // namespace ui

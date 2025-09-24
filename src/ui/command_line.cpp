#include "command_line.hpp"

#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "core/command_line.hpp"
#include "core/context.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "core/severity.hpp"
#include "ui/palette.hpp"
#include "ui/picker_entry.hpp"
#include "ui/text_box.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

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

CommandLine::~CommandLine() = default;

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

}  // namespace ui

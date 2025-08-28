#include "command_line.hpp"

#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/command_line.hpp"
#include "core/message_line.hpp"
#include "core/severity.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/text_box.hpp"

using namespace ftxui;

namespace ui
{

struct CommandLine::Impl final
{
    Impl(core::Context& context);
    Element render(Ftxui& ui, core::Context& context);

    ftxui::Component input;
};

CommandLine::Impl::Impl(core::Context& context)
    : input(TextBox({
        .content = &context.commandLine.line,
        .cursorPosition = &context.commandLine.cursor,
        .suggestion = &context.commandLine.suggestion,
        .suggestionColor = Palette::bg5
    }))
{
}

Element CommandLine::Impl::render(Ftxui& ui, core::Context& context)
{
    if (ui.active->type == UIComponent::commandLine)
    {
        auto& completions = context.commandLine.completions;

        if (completions.empty())
        {
            return hbox(text(":"), input->Render());
        }

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
                | bgcolor(Palette::StatusLine::bg1)
        };

        for (auto it = completions.begin(); it != completions.end(); ++it)
        {
            if (it == context.commandLine.currentCompletion)
            {
                elements.push_back(text(std::string(*it)) | inverted);
            }
            else
            {
                elements.push_back(text(std::string(*it)));
            }
            elements.push_back(separatorCharacter(" "));
        }

        return vbox(
                hbox(std::move(elements)),
                hbox(text(":"), input->Render()));
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

CommandLine::CommandLine(core::Context& context)
    : UIComponent(UIComponent::commandLine)
    , pimpl_(new Impl(context))
{
}

CommandLine::~CommandLine()
{
    delete pimpl_;
}

void CommandLine::takeFocus()
{
    pimpl_->input->TakeFocus();
}

ftxui::Element CommandLine::render(core::Context& context)
{
    return pimpl_->render(context.ui->get<Ftxui>(), context);
}

CommandLine::operator ftxui::Component&()
{
    return pimpl_->input;
}

}  // namespace ui

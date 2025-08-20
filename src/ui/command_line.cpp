#include "command_line.hpp"

#include <cstring>
#include <spanstream>
#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/command_line.hpp"
#include "core/severity.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"

using namespace ftxui;

namespace ui
{

struct CommandLine::Impl final
{
    Impl(core::Context& context);

    Severity         severity;
    char             buffer[256];
    std::ospanstream messageLine;
    ftxui::Component input;

    Element render(Ftxui& ui, core::Context& context);
    void clearMessageLine();
    std::ostream& getMessageStream(Severity severity);
};

CommandLine::Impl::Impl(core::Context& context)
    : severity(info)
    , buffer{}
    , messageLine(buffer)
    , input(
        Input(
            &context.commandLine.line,
            InputOption{
                .multiline = false,
                .cursor_position = reinterpret_cast<int*>(&context.commandLine.cursor)
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

    switch (severity)
    {
        case error:
            return color(Color::Red, text(messageLine.span().data()));
        case warning:
            return color(Color::Yellow, text(messageLine.span().data()));
        default:
            return text(messageLine.span().data());
    }
}

void CommandLine::Impl::clearMessageLine()
{
    std::memset(buffer, 0, sizeof(pimpl_->buffer));
    std::ospanstream s(buffer);
    messageLine.swap(s);
}

std::ostream& CommandLine::Impl::getMessageStream(Severity s)
{
    clearMessageLine();
    severity = s;
    return messageLine;
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

void CommandLine::clearMessageLine()
{
    pimpl_->clearMessageLine();
}

std::ostream& CommandLine::operator<<(Severity severity)
{
    return pimpl_->getMessageStream(severity);
}

CommandLine::operator ftxui::Component&()
{
    return pimpl_->input;
}

}  // namespace ui

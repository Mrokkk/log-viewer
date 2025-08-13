#include "command_line.hpp"

#include <cstring>
#include <spanstream>
#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/interpreter.hpp"
#include "ui/event_handler.hpp"
#include "ui/ftxui.hpp"
#include "core/logger.hpp"

using namespace ftxui;

namespace ui
{

struct CommandLine::Impl final
{
    Impl();

    Severity         severity;
    char             buffer[256];
    std::string      inputLine;
    std::ospanstream messageLine;
    ftxui::Component input;
    EventHandlers    eventHandlers;

    Element render(Ftxui& ui);
    bool handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context);
    void clearMessageLine();
    std::ostream& getMessageStream(Severity severity);

private:
    bool accept(Ftxui& ui, core::Context& context);
    bool escape();
};

CommandLine::Impl::Impl()
    : severity(info)
    , buffer{}
    , messageLine(buffer)
    , input(Input(&inputLine, InputOption{.multiline = false}))
    , eventHandlers{
        {Event::Return, [this](auto& ui, auto& context){ return accept(ui, context); }}
    }
{
}

Element CommandLine::Impl::render(Ftxui& ui)
{
    if (ui.active->type == UIComponent::commandLine)
    {
        return hbox(text(":"), input->Render());
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

bool CommandLine::Impl::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    // Ignore mouse events so that command line cannot be escaped
    if (event.is_mouse())
    {
        return true;
    }

    auto result = eventHandlers.handleEvent(event, ui, context);

    if (not result)
    {
        return input->OnEvent(event);
    }

    return result.value();
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

bool CommandLine::Impl::accept(Ftxui& ui, core::Context& context)
{
    core::executeCode(inputLine, context);
    // If command already changed active, do not change it
    if (ui.active->type == UIComponent::commandLine)
    {
        switchFocus(UIComponent::mainView, ui, context);
    }
    return true;
}

bool CommandLine::Impl::escape()
{
    clearMessageLine();
    return true;
}

CommandLine::CommandLine()
    : UIComponent(UIComponent::commandLine)
    , pimpl_(new Impl)
{
}

CommandLine::~CommandLine()
{
    delete pimpl_;
}

void CommandLine::takeFocus()
{
    pimpl_->clearMessageLine();
    pimpl_->inputLine.clear();
    pimpl_->input->TakeFocus();
}

ftxui::Element CommandLine::render(core::Context& context)
{
    return pimpl_->render(context.ui->get<Ftxui>());
}

bool CommandLine::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    return pimpl_->handleEvent(event, ui, context);
}

void CommandLine::clearMessageLine()
{
    pimpl_->clearMessageLine();
}

void CommandLine::clearInputLine()
{
    pimpl_->inputLine.clear();
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

#include "status_line.hpp"

#include "core/context.hpp"
#include "core/input.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/ui_component.hpp"

using namespace ftxui;

namespace ui
{

Element renderStatusLine(Ftxui& ui, core::Context& context)
{
    const auto fileName{ui.mainView.activeFileName(context)};
    Color statusFg, statusBg;
    const char* status;

    if (context.mode == core::Mode::command)
    {
        statusFg = Palette::StatusLine::commandFg;
        statusBg = Palette::StatusLine::commandBg;
        status = " COMMAND ";
    }
    else if (context.mode == core::Mode::visual)
    {
        statusFg = Palette::StatusLine::visualFg;
        statusBg = Palette::StatusLine::visualBg;
        status = " VISUAL ";
    }
    else if (context.mode == core::Mode::normal)
    {
        statusFg = Palette::StatusLine::normalFg;
        statusBg = Palette::StatusLine::normalBg;
        status = " NORMAL ";
    }
    else
    {
        switch (ui.active->type)
        {
            default:
                status = " UNEXPECTED ";
                break;
            case UIComponent::picker:
                status = " PICKER ";
                break;
            case UIComponent::grepper:
                status = " GREPPER ";
                break;
        }
        statusFg = Palette::StatusLine::normalFg;
        statusBg = Palette::StatusLine::normalBg;
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
            | color(Palette::StatusLine::bg3)
            | bgcolor(Palette::StatusLine::bg1),
        text(" ")
            | bgcolor(Palette::StatusLine::bg3),
        text(core::inputStateString(context))
            | bgcolor(Palette::StatusLine::bg3),
        text(" ")
            | bgcolor(Palette::StatusLine::bg3),
    });
}

}  // namespace ui

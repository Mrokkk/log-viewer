#include "status_line.hpp"

#include "core/context.hpp"
#include "core/input.hpp"
#include "core/main_view.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "utils/buffer.hpp"

using namespace ftxui;

namespace ui
{

Element renderStatusLine(Ftxui&, core::Context& context)
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

}  // namespace ui

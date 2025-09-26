#include "grepper.hpp"

#include <string_view>

#include <ftxui/dom/elements.hpp>

#include "core/context.hpp"
#include "core/grep_options.hpp"
#include "core/grepper.hpp"
#include "core/readline.hpp"
#include "ui/palette.hpp"
#include "ui/text_box.hpp"
#include "utils/buffer.hpp"

using namespace ftxui;

namespace ui
{

Grepper::Grepper(core::Context& context)
    : mTextBox({
        .content = context.grepper.readline.line(),
        .cursorPosition = &context.grepper.readline.cursor(),
        .suggestion = &context.grepper.readline.suggestion(),
        .suggestionColor = &Palette::bg5
    })
{
}

Grepper::~Grepper() = default;

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

}  // namespace ui

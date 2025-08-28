#include "text_box.hpp"

#include <string>
#include <utility>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/string_internal.hpp"

using namespace ftxui;

namespace ui
{

namespace
{

struct TextBoxBase final : public ComponentBase, public TextBoxOptions
{
    TextBoxBase(TextBoxOptions option)
        : TextBoxOptions(std::move(option))
    {
    }

private:
    Element OnRender() override
    {
        if (not cursorPosition)
        {
            return text(*content);
        }

        if (content->empty())
        {
            return hbox(text(" ") | inverted);
        }

        Element element;

        if (*cursorPosition >= content->size())
        {
            if (suggestion and not suggestion->empty())
            {
                element = hbox({
                    text(*content),
                    text(std::string{suggestion->at(0)}) | inverted,
                    text(suggestion->substr(1)) | color(suggestionColor),
                });
            }
            else
            {
                element = hbox({
                    text(*content),
                    text(" ") | inverted,
                });
            }
        }
        else
        {
            const int glyphStart = *cursorPosition;
            const int glyphEnd = static_cast<int>(GlyphNext(*content, glyphStart));
            const auto partBeforeCursor = content->substr(0, glyphStart);
            const auto partAtCursor = content->substr(glyphStart, glyphEnd - glyphStart);
            const auto partAfterCursor = content->substr(glyphEnd);

            Elements e = {
                text(partBeforeCursor),
                text(partAtCursor) | inverted,
                text(partAfterCursor)
            };

            if (suggestion and not suggestion->empty())
            {
                e.push_back(text(suggestion) | color(suggestionColor));
            }

            element = hbox(e);
        }

        return element;
    }

    bool Focusable() const override final
    {
        return false;
    }
};

}  // namespace

Component TextBox(TextBoxOptions option)
{
    return Make<TextBoxBase>(std::move(option));
}

}  // namespace ui

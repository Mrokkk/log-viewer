#include "text_box.hpp"

#include <string>
#include <utility>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string_internal.hpp>

using namespace ftxui;

namespace ui
{

Element renderTextBox(const TextBox& options)
{
    if (not options.cursorPosition)
    {
        return text(&options.content);
    }

    if (options.content.empty())
    {
        return hbox(text(" ") | inverted);
    }

    if (*options.cursorPosition >= options.content.size())
    {
        if (options.suggestion and not options.suggestion->empty())
        {
            return hbox({
                text(&options.content),
                text(std::string{options.suggestion->at(0)}) | inverted,
                text(
                    options.suggestion->substr(1))
                        | color(options.suggestionColor
                            ? *options.suggestionColor
                            : Color()),
            });
        }
        else
        {
            return hbox({
                text(&options.content),
                text(" ") | inverted,
            });
        }
    }
    else
    {
        const int glyphStart = *options.cursorPosition;
        const int glyphEnd = static_cast<int>(GlyphNext(options.content, glyphStart));
        const auto partBeforeCursor = options.content.substr(0, glyphStart);
        const auto partAtCursor = options.content.substr(glyphStart, glyphEnd - glyphStart);
        const auto partAfterCursor = options.content.substr(glyphEnd);

        Elements e = {
            text(partBeforeCursor),
            text(partAtCursor) | inverted,
            text(partAfterCursor)
        };

        if (options.suggestion and not options.suggestion->empty())
        {
            e.push_back(
                text(*options.suggestion)
                    | color(options.suggestionColor
                        ? *options.suggestionColor
                        : Color()));
        }

        return hbox(std::move(e));
    }
}

}  // namespace ui

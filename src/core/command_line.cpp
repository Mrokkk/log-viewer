#include "command_line.hpp"

#include <flat_set>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/input.hpp"
#include "core/interpreter.hpp"
#include "utils/string.hpp"

namespace core
{

CommandLine::CommandLine()
    : line(readline.lineRef())
    , cursor(readline.cursorRef())
    , completions(readline.completions())
    , currentCompletion(readline.currentCompletion())
    , suggestion(readline.suggestionRef())
{
    readline
        .enableSuggestions()
        .onAccept(
            [this](InputSource, Context& context)
            {
                accept(context);
            })
        .setupCompletion(
            [](std::string_view pattern)
            {
                std::flat_set<std::string_view> result;
                for (const auto& [_, command] : Commands::map())
                {
                    if (command.name.starts_with(pattern))
                    {
                        result.insert(command.name);
                    }
                }
                for (const auto& [_, alias] : Aliases::map())
                {
                    if (alias.name.starts_with(pattern))
                    {
                        result.insert(alias.name);
                    }
                }
                return utils::StringViews(result.begin(), result.end());
            });
}

CommandLine::~CommandLine() = default;

void CommandLine::accept(Context& context)
{
    executeCode(line, context);
    readline.clear();
}

void enterCommandLine(InputSource source, Context& context)
{
    auto& commandLine = context.commandLine;
    commandLine.readline.clear();
    if (source == InputSource::user)
    {
        commandLine.readline.refreshCompletion();
    }
}

bool handleCommandLineKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    return context.commandLine.readline.handleKeyPress(keyPress, source, context);
}

void clearCommandLineHistory(Context& context)
{
    context.commandLine.readline.clearHistory();
}

}  // namespace core

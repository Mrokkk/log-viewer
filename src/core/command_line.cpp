#include "command_line.hpp"

#include <algorithm>
#include <flat_set>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/dirs.hpp"
#include "core/input.hpp"
#include "core/interpreter.hpp"
#include "core/readline.hpp"
#include "utils/string.hpp"

namespace core
{

static utils::Strings feedFiles(Context&)
{
    return readCurrentDirectoryRecursive();
}

static utils::Strings feedHistory(CommandLine& commandLine)
{
    auto history = commandLine.readline.history();
    std::reverse(history.begin(), history.end());
    return history;
}

CommandLine::CommandLine()
    : mFilesPicker(&feedFiles)
    , mHistoryPicker([this](auto&){ return feedHistory(*this); })
{
    readline
        .enableSuggestions()
        .connectPicker(mFilesPicker, 't', Readline::AcceptBehaviour::append)
        .connectPicker(mHistoryPicker, 'r')
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
    executeCode(readline.line(), context);
    readline.clear();
}

void CommandLine::enter(InputSource source)
{
    readline.clear();
    if (source == InputSource::user)
    {
        readline.refreshCompletion();
    }
}

bool CommandLine::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    return context.commandLine.readline.handleKeyPress(keyPress, source, context);
}

void CommandLine::clearHistory()
{
    readline.clearHistory();
}

}  // namespace core

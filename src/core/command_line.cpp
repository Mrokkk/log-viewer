#include "command_line.hpp"

#include <algorithm>
#include <flat_set>

#include "core/alias.hpp"
#include "core/context.hpp"
#include "core/dirs.hpp"
#include "core/input.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/interpreter.hpp"
#include "core/main_view.hpp"
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
    auto history = commandLine.readline().history();
    std::reverse(history.begin(), history.end());
    return history;
}

CommandLine::CommandLine()
    : mMode(Mode::command)
    , mFilesPicker(&feedFiles)
    , mHistoryPicker([this](auto&){ return feedHistory(*this); })
{
    commandReadline
        .enableSuggestions()
        .connectPicker(mFilesPicker, 't', Readline::AcceptBehaviour::append)
        .connectPicker(mHistoryPicker, 'r')
        .onAccept(
            [this](InputSource, Context& context)
            {
                acceptCommand(context);
            })
        .setupCompletion(
            [](std::string_view pattern)
            {
                std::flat_set<std::string_view> result;
                interpreter::Commands::forEach(
                    [&result, &pattern](const interpreter::Command& command)
                    {
                        if (command.name.starts_with(pattern))
                        {
                            result.insert(command.name);
                        }
                    });
                Aliases::forEach(
                    [&result, &pattern](const Alias& alias)
                    {
                        if (alias.name.starts_with(pattern))
                        {
                            result.insert(alias.name);
                        }
                    });
                return utils::StringViews(result.begin(), result.end());
            });

    searchReadline
        .connectPicker(mHistoryPicker, 'r')
        .onAccept(
            [this](InputSource, Context& context)
            {
                acceptSearch(context);
            });
}

CommandLine::~CommandLine() = default;

void CommandLine::acceptCommand(Context& context)
{
    interpreter::execute(commandReadline.line(), context);
    commandReadline.clear();
}

void CommandLine::acceptSearch(Context& context)
{
    if (mMode == Mode::searchForward)
    {
        context.mainView.searchForward(searchReadline.line(), context);
    }
    else
    {
        context.mainView.searchBackward(searchReadline.line(), context);
    }
    searchReadline.clear();
}

void CommandLine::enter(InputSource source, Mode mode)
{
    mMode = mode;
    auto& r = readline();
    r.clear();
    if (source == InputSource::user)
    {
        r.refreshCompletion();
    }
}

bool CommandLine::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    return context.commandLine.readline().handleKeyPress(keyPress, source, context);
}

void CommandLine::clearHistory()
{
    commandReadline.clearHistory();
}

}  // namespace core

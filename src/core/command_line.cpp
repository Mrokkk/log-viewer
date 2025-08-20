#include "command_line.hpp"

#include <flat_set>
#include <string>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/input.hpp"
#include "core/interpreter.hpp"
#include "utils/string.hpp"

namespace core
{

CommandLine::CommandLine()
    : cursor(0)
    , currentCompletion(completions.end())
{
}

CommandLine::~CommandLine() = default;

enum class Completion
{
    forward,
    backward,
};

struct CommandLineOps final : CommandLine
{
    CommandLineOps() = delete;
    ~CommandLineOps() = delete;

    void clear();
    void clearCompletions();
    void moveCursorLeft();
    void moveCursorRight();
    bool write(char c);
    bool write(const std::string& string);
    void selectPrevHistoryEntry();
    void selectNextHistoryEntry();
    void jumpToBeginning();
    void jumpToEnd();
    void jumpToPrevWord();
    void jumpToNextWord();
    bool erasePrevCharacter();
    bool eraseNextCharacter();
    bool eraseWord();
    void accept(Context& context, InputSource source);
    void refreshCompletion();
    void complete(Completion type);
};

void CommandLineOps::clear()
{
    line.clear();
    cursor = 0;
    clearCompletions();
}

void CommandLineOps::clearCompletions()
{
    completions.clear();
    savedLine.clear();
    currentCompletion = completions.end();
}

void CommandLineOps::moveCursorLeft()
{
    if (cursor > 0)
    {
        --cursor;
    }
}

void CommandLineOps::moveCursorRight()
{
    if (cursor < line.size())
    {
        ++cursor;
    }
}

bool CommandLineOps::write(char c)
{
    line.insert(cursor++, 1, c);
    return true;
}

bool CommandLineOps::write(const std::string& string)
{
    line.insert(cursor, string);
    cursor += string.size();
    return true;
}

void CommandLineOps::selectPrevHistoryEntry()
{
    if (history.current.isBeginning())
    {
        return;
    }
    if (not history.current)
    {
        savedLine = line;
    }
    --history.current;
    line = *history.current;
    cursor = line.size();
    clearCompletions();
}

void CommandLineOps::selectNextHistoryEntry()
{
    if (++history.current)
    {
        line = *history.current;
        cursor = line.size();
    }
    else
    {
        line = savedLine;
        cursor = line.size();
    }
    clearCompletions();
}

void CommandLineOps::jumpToBeginning()
{
    cursor = 0;
}

void CommandLineOps::jumpToEnd()
{
    cursor = line.size();
}

void CommandLineOps::jumpToPrevWord()
{
    if (cursor > 0)
    {
        if (line[cursor - 1] == ' ')
        {
            --cursor;
        }
        if (cursor > 0)
        {
            auto it = line.rfind(' ', --cursor);
            cursor = it != std::string::npos
                ? it
                : 0;
        }
    }
}

void CommandLineOps::jumpToNextWord()
{
    if (cursor < line.size())
    {
        auto it = line.find(' ', ++cursor);
        cursor = it != std::string::npos
            ? it
            : line.size();
    }
}

bool CommandLineOps::erasePrevCharacter()
{
    if (cursor > 0)
    {
        line.erase(--cursor, 1);
        return true;
    }
    return false;
}

bool CommandLineOps::eraseNextCharacter()
{
    if (cursor < line.size())
    {
        line.erase(cursor, 1);
        return true;
    }
    return false;
}

bool CommandLineOps::eraseWord()
{
    if (cursor > 0)
    {
        auto it = line.rfind(' ', cursor);
        if (it == std::string::npos)
        {
            it = 0;
        }
        else
        {
            while (it)
            {
                if (line[it - 1] != ' ')
                {
                    break;
                }
                it--;
            }
        }
        line.erase(it, cursor - it);
        return true;
    }
    return false;
}

void CommandLineOps::accept(Context& context, InputSource source)
{
    if (source == InputSource::user)
    {
        history.pushBack(line);
    }
    executeCode(line, context);
    clear();
}

void CommandLineOps::refreshCompletion()
{
    const auto& pattern = line;
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
    completions = utils::StringViews(result.begin(), result.end());
    currentCompletion = completions.end();
}

void CommandLineOps::complete(Completion type)
{
    if (not completions.empty())
    {
        utils::StringViews::iterator pivot;
        utils::StringViews::iterator nextPivot;
        utils::StringViews::iterator next;

        switch (type)
        {
            case Completion::forward:
                pivot = completions.end();
                nextPivot = completions.begin();
                next = currentCompletion + 1;
                break;
            case Completion::backward:
                pivot = completions.begin();
                nextPivot = completions.end();
                next = currentCompletion - 1;
                break;
        }

        if (currentCompletion == pivot)
        {
            currentCompletion = nextPivot;
        }
        else
        {
            currentCompletion = next;
        }

        if (currentCompletion != completions.end())
        {
            line = *currentCompletion;
            cursor = line.size();
        }
    }
}

static CommandLineOps& getImpl(core::Context& context)
{
    return static_cast<CommandLineOps&>(context.commandLine);
}

void enterCommandLine(InputSource source, core::Context& context)
{
    auto& commandLine = getImpl(context);
    commandLine.clear();
    if (source == InputSource::user)
    {
        commandLine.refreshCompletion();
    }
}

bool handleCommandLineKeyPress(KeyPress keyPress, InputSource source, core::Context& context)
{
    auto& commandLine = getImpl(context);
    bool requireCompletionUpdate{false};

    switch (keyPress.type)
    {
        case KeyPress::Type::character:
            requireCompletionUpdate = commandLine.write(keyPress.value);
            break;

        case KeyPress::Type::arrowLeft:
            commandLine.moveCursorLeft();
            break;

        case KeyPress::Type::arrowRight:
            commandLine.moveCursorRight();
            break;

        case KeyPress::Type::arrowUp:
            commandLine.selectPrevHistoryEntry();
            break;

        case KeyPress::Type::arrowDown:
            commandLine.selectNextHistoryEntry();
            break;

        case KeyPress::Type::ctrlArrowLeft:
            commandLine.jumpToPrevWord();
            break;

        case KeyPress::Type::ctrlArrowRight:
            commandLine.jumpToNextWord();
            break;

        case KeyPress::Type::home:
            commandLine.jumpToBeginning();
            break;

        case KeyPress::Type::end:
            commandLine.jumpToEnd();
            break;

        case KeyPress::Type::tab:
            commandLine.complete(Completion::forward);
            break;

        case KeyPress::Type::shiftTab:
            commandLine.complete(Completion::backward);
            break;

        case KeyPress::Type::backspace:
            requireCompletionUpdate = commandLine.erasePrevCharacter();
            break;

        case KeyPress::Type::del:
            requireCompletionUpdate = commandLine.eraseNextCharacter();
            break;

        case KeyPress::Type::ctrlCharacter:
            switch (keyPress.value)
            {
                case 'c':
                    commandLine.clear();
                    return true;

                case 'w':
                    requireCompletionUpdate = commandLine.eraseWord();
                    break;
            }
            break;

        case KeyPress::Type::cr:
            commandLine.accept(context, source);
            return true;

        default:
            requireCompletionUpdate = commandLine.write(keyPress.name());
            break;
    }

    if (requireCompletionUpdate and source == InputSource::user)
    {
        commandLine.refreshCompletion();
    }

    return false;
}

History::History()
    : current(content)
{
}

History::~History() = default;

void History::pushBack(const std::string& entry)
{
    content.push_back(entry);
    current.reset();
}

void History::clear()
{
    content.clear();
    current.reset();
}

History::Iterator::Iterator(utils::Strings& content)
    : content_(content)
    , iterator_(content_.end())
{
}

History::Iterator::~Iterator() = default;

bool History::Iterator::isBeginning() const
{
    return iterator_ == content_.begin();
}

bool History::Iterator::isEnd() const
{
    return iterator_ == content_.end();
}

History::Iterator::operator bool() const
{
    return iterator_ != content_.end();
}

const std::string& History::Iterator::operator*() const
{
    return *iterator_;
}

History::Iterator& History::Iterator::operator--()
{
    if (iterator_ != content_.begin())
    {
        --iterator_;
    }
    return *this;
}

History::Iterator& History::Iterator::operator++()
{
    if (iterator_ != content_.end())
    {
        ++iterator_;
    }
    return *this;
}

void History::Iterator::reset()
{
    iterator_ = content_.end();
}

}  // namespace core

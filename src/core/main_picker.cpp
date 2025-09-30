#include "main_picker.hpp"

#include <cstdlib>
#include <ctime>

#include "core/commands/open.hpp"
#include "core/dirs.hpp"
#include "core/event.hpp"
#include "core/event_handler.hpp"
#include "core/events/resize.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/symbols_map.hpp"
#include "core/logger.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"
#include "utils/time_format.hpp"

namespace core
{

MainPicker::MainPicker()
    : mCurrentPicker(0)
    , mPickers{
        Picker(Picker::Orientation::topDown, feedFiles),
        Picker(Picker::Orientation::topDown, feedBookmarks),
        Picker(Picker::Orientation::topDown, feedCommands),
        Picker(Picker::Orientation::topDown, feedVariables),
        Picker(Picker::Orientation::topDown, feedMessages),
        Picker(Picker::Orientation::topDown, feedLogs),
    }
{
    registerEventHandler(
        Event::Type::Resize,
        [this](EventPtr event, InputSource, Context& context)
        {
            auto& ev = event->cast<events::Resize>();
            resize(ev.resx, ev.resy, context);
        });

    mReadline.onAccept(
        [this](InputSource, Context& context)
        {
            accept(context);
        });
}

MainPicker::~MainPicker() = default;

void MainPicker::enter(Context& context, Type type)
{
    mCurrentPicker = static_cast<int>(type);
    mReadline.clear();
    mReadline.connectPicker(mPickers[mCurrentPicker], context);
}

bool MainPicker::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    if (keyPress == KeyPress::tab)
    {
        auto nextType = static_cast<Type>(mCurrentPicker + 1);

        if (nextType == Type::_last)
        {
            nextType = Type::files;
        }

        mCurrentPicker = nextType;
        mReadline.clear();
        mReadline.connectPicker(mPickers[mCurrentPicker], context);

        return false;
    }
    else if (keyPress == KeyPress::shiftTab)
    {
        auto nextType = static_cast<Type>(mCurrentPicker - 1);

        if (static_cast<int>(nextType < 0))
        {
            nextType = static_cast<Type>(Type::_last - 1);
        }

        mCurrentPicker = nextType;
        mReadline.clear();
        mReadline.connectPicker(mPickers[mCurrentPicker], context);

        return false;
    }

    return mReadline.handleKeyPress(keyPress, source, context);
}

void MainPicker::resize(int, int resy, Context&)
{
    for (auto& picker : mPickers)
    {
        picker.setHeight(resy / 2);
    }
}

void MainPicker::accept(Context& context)
{
    switch (static_cast<Type>(mCurrentPicker))
    {
        case Type::files:
            core::commands::open(mReadline.line(), context);
            break;

        case Type::bookmarks:
        {
            auto line = strtoul(mReadline.line().c_str(), NULL, 10);
            context.mainView.scrollToAbsolute(line, context);
            break;
        }

        case Type::commands:
        case Type::variables:
        case Type::messages:
        case Type::logs:
        default:
            break;
    }
    mReadline.clear();
}

utils::Strings MainPicker::feedFiles(Context&)
{
    return core::readCurrentDirectoryRecursive();
}

utils::Strings MainPicker::feedBookmarks(Context& context)
{
    auto& mainView = context.mainView;
    auto windowNode = mainView.currentWindowNode();
    if (not windowNode)
    {
        return {};
    }

    auto& w = windowNode->window();

    utils::Strings strings;
    strings.reserve(w.bookmarks->size());

    for (const auto& bookmark : *w.bookmarks)
    {
        utils::Buffer buf;
        buf << bookmark.lineNumber << ": " << bookmark.name;
        strings.emplace_back(buf.str());
    }

    return strings;
}

utils::Strings MainPicker::feedCommands(Context&)
{
    utils::Strings strings;
    interpreter::Commands::forEach(
        [&strings](const core::interpreter::Command& command)
        {
            utils::Buffer commandDesc;

            commandDesc << command.name << ' ';

            for (const auto& flag : command.flags.get())
            {
                commandDesc << "[-" << flag.name << "] ";
            }

            for (const auto& arg : command.arguments.get())
            {
                if (arg.type == core::Type::variadic)
                {
                    commandDesc << '[' << arg.name << "]... ";
                }
                else
                {
                    commandDesc << '[' << arg.type << ':' << arg.name << "] ";
                }
            }

            strings.emplace_back(commandDesc.str());
        });
    return strings;
}

utils::Strings MainPicker::feedVariables(Context&)
{
    utils::Strings strings;
    core::interpreter::symbolsMap().forEach(
        [&strings](const core::interpreter::SymbolsMap::Node& node)
        {
            auto& variable = *node.second;
            auto& variableName = node.first;

            utils::Buffer variableDesc;
            variableDesc << variableName << '{' << variable.value().type() << "} : " << variable;

            strings.emplace_back(variableDesc.str());
        });
    return strings;
}

utils::Strings MainPicker::feedMessages(Context& context)
{
    return context.messageLine.history();
}

utils::Strings MainPicker::feedLogs(Context&)
{
    utils::Strings strings;
    logger.forEachLogEntry(
        [&strings](const LogEntry& entry)
        {
            utils::Buffer buf;

            buf << utils::formatTime(std::localtime(&entry.time), "%F %T");

            if (entry.header)
            {
                buf << " [" << entry.header << ']';
            }

            buf << ' ' << entry.message;

            strings.emplace_back(buf.str());
        });
    return strings;
}

}  // namespace core

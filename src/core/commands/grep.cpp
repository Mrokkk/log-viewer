#include "core/buffer.hpp"
#include "core/command.hpp"
#include "core/grep_options.hpp"
#include "core/interpreter.hpp"
#include "core/main_loop.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "utils/buffer.hpp"

namespace core
{

DEFINE_COMMAND(grep)
{
    HELP() = "grep current buffer";

    FLAGS()
    {
        return {
            "c",
            "i",
            "r",
        };
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "pattern")
        };
    }

    EXECUTOR()
    {
        auto parentWindow = context.mainView.currentWindowNode();

        if (not parentWindow) [[unlikely]]
        {
            context.messageLine.error() << "No buffer loaded yet";
            return false;
        }

        auto pattern = args[0].string;

        GrepOptions options;
        std::string optionsString;

        if (flags.contains("r"))
        {
            options.regex = true;
            optionsString += 'r';
        }
        if (flags.contains("c"))
        {
            options.caseInsensitive = true;
            optionsString += 'c';
        }
        if (flags.contains("i"))
        {
            options.inverted = true;
            optionsString += 'i';
        }

        utils::Buffer buf;

        buf << pattern;

        if (not optionsString.empty())
        {
            buf << " [" << optionsString << ']';
        }

        auto& newWindow = context.mainView.createWindow(buf.str(), MainView::Parent::currentWindow, context);

        auto newBuffer = newWindow.buffer();

        newBuffer->grep(
            std::move(pattern),
            options,
            parentWindow->bufferId(),
            context,
            [&newWindow, &context](TimeOrError result)
            {
                if (result)
                {
                    auto newBuffer = newWindow.buffer();

                    if (not newBuffer)
                    {
                        return;
                    }

                    context.messageLine.info()
                        << "found " << newBuffer->lineCount() << " matches; took "
                        << (result.value() | utils::precision(3)) << " s";

                    context.mainLoop->executeTask(
                        [&newWindow, &context]
                        {
                            context.mainView.bufferLoaded(newWindow, context);
                        });
                }
                else if (context.running)
                {
                    context.messageLine.error() << "Error grepping buffer: " << result.error();
                    context.mainView.removeWindow(*newWindow.parent(), context);
                }
            });

        return true;
    }
}

namespace commands
{

bool grep(const std::string& pattern, const GrepOptions& options, Context& context)
{
    std::string command = "grep ";

    if (options.regex)
    {
        command += "-r ";
    }
    if (options.caseInsensitive)
    {
        command += "-c ";
    }
    if (options.inverted)
    {
        command += "-i ";
    }

    command += '\"';
    command += pattern;
    command += '\"';

    return executeCode(command, context);
}

}  // namespace commands

}  // namespace core

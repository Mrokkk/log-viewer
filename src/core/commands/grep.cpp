#include "core/command.hpp"
#include "core/grep_options.hpp"
#include "core/interpreter.hpp"
#include "core/message_line.hpp"
#include "core/user_interface.hpp"
#include "core/view.hpp"
#include "utils/buffer.hpp"

namespace core
{

DEFINE_COMMAND(grep)
{
    HELP() = "grep current view";

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
        auto pattern = args[0].string;
        GrepOptions options;

        if (flags.contains("r"))
        {
            options.regex = true;
        }
        if (flags.contains("c"))
        {
            options.caseInsensitive = true;
        }
        if (flags.contains("i"))
        {
            options.inverted = true;
        }

        auto parentViewId = context.ui->getCurrentView();

        auto parentView = getView(parentViewId, context);

        if (not parentView) [[unlikely]]
        {
            context.messageLine.error() << "no view loaded yet";
            return false;
        }

        std::string optionsString;

        if (options.regex)
        {
            optionsString += 'r';
        }
        if (options.caseInsensitive)
        {
            optionsString += 'c';
        }
        if (options.inverted)
        {
            optionsString += 'i';
        }

        utils::Buffer buf;

        buf << pattern;

        if (not optionsString.empty())
        {
            buf << " [" << optionsString << ']';
        }

        auto [newView, newViewId] = context.views.allocate();

        auto uiView = context.ui->createView(buf.str(), newViewId, Parent::currentView, context);

        if (uiView.expired()) [[unlikely]]
        {
            context.messageLine.error() << "failed to create UI view";
            context.views.free(newViewId);
            return false;
        }

        newView.grep(
            std::move(pattern),
            options,
            parentViewId,
            context,
            [uiView, newViewId, &context](TimeOrError result)
            {
                if (result)
                {
                    auto newView = getView(newViewId, context);

                    if (newView)
                    {
                        context.messageLine.info()
                            << "found " << newView->lineCount() << " matches; took "
                            << (result.value() | utils::precision(3)) << " s";

                        context.ui->onViewDataLoaded(uiView, context);
                    }
                }
                else if (context.running)
                {
                    context.messageLine.error() << "Error grepping view: " << result.error();
                    context.ui->removeView(uiView, context);
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

#include <iomanip>

#include "core/command.hpp"
#include "core/grep_options.hpp"
#include "core/interpreter.hpp"
#include "core/message_line.hpp"
#include "core/severity.hpp"
#include "core/user_interface.hpp"
#include "core/view.hpp"

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
            context.messageLine << error << "no view loaded yet";
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

        char buffer[128];
        std::spanstream ss(buffer);

        ss << pattern;

        if (not optionsString.empty())
        {
            ss << " [" << optionsString << ']';
        }

        auto [newView, newViewId] = context.views.allocate();

        auto uiView = context.ui->createView(ss.span().data(), newViewId, Parent::currentView, context);

        if (uiView.expired()) [[unlikely]]
        {
            context.messageLine << error << "failed to create UI view";
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
                        context.messageLine << info
                            << "found " << newView->lineCount() << " matches; took "
                            << std::fixed << std::setprecision(3) << result.value() << " s";

                        context.ui->onViewDataLoaded(uiView, context);
                    }
                }
                else if (context.running)
                {
                    context.messageLine << error << "Error grepping view: " << result.error();
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

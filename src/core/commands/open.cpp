#include "open.hpp"

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/interpreter.hpp"
#include "core/message_line.hpp"
#include "core/view.hpp"
#include "core/user_interface.hpp"
#include "utils/buffer.hpp"

namespace core
{

DEFINE_COMMAND(open)
{
    HELP() = "open a file";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "path")
        };
    };

    EXECUTOR()
    {
        constexpr auto errorHeader = "Error loading file: ";
        auto& path = args[0].string;

        auto [newView, newViewId] = context.views.allocate();

        auto uiView = context.ui->createView(path, newViewId, Parent::root, context);

        if (uiView.expired()) [[unlikely]]
        {
            context.messageLine.error() << errorHeader << "Failed to create UI view";
            context.views.free(newViewId);
            return false;
        }

        newView.load(
            path,
            context,
            [uiView, newViewId, &context, errorHeader](TimeOrError result)
            {
                if (result)
                {
                    auto newView = getView(newViewId, context);

                    if (not newView)
                    {
                        return;
                    }

                    context.messageLine.info()
                        << newView->filePath() << ": lines: " << newView->lineCount() << "; took "
                        << (result.value() | utils::precision(3)) << " s";

                    context.ui->onViewDataLoaded(uiView, context);
                }
                else if (context.running)
                {
                    context.messageLine.error() << errorHeader << result.error();
                    context.ui->removeView(uiView, context);
                }
            });

        return true;
    }
}

DEFINE_ALIAS(e, open);

namespace commands
{

bool open(const std::string& path, Context& context)
{
    utils::Buffer buf;
    buf << open::Command::name << " \"" << path << "\"";
    return executeCode(buf.str(), context);
}

}  // namespace commands

}  // namespace core

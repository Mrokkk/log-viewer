#include "core/buffer.hpp"
#include "core/command.hpp"
#include "core/main_loop.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"

namespace core
{

DEFINE_COMMAND(filter)
{
    HELP() = "filter current view";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto parentWindow = context.mainView.currentWindowNode();

        if (not parentWindow) [[unlikely]]
        {
            context.messageLine.error() << "No buffer loaded yet";
            return false;
        }

        auto& w = parentWindow->window();

        if (not w.selectionMode)
        {
            context.messageLine.error() << "Nothing selected";
            return false;
        }

        size_t start = w.selectionStart;
        size_t end = w.selectionEnd;

        utils::Buffer buf;
        buf << '<' << start << '-' << end << '>';

        auto& newWindow = context.mainView.createWindow(buf.str(), MainView::Parent::currentWindow, context);

        auto newBuffer = newWindow.buffer();

        parentWindow->window().selectionMode = false;

        newBuffer->filter(
            start,
            end,
            parentWindow->bufferId(),
            context,
            [&newWindow, &context](core::TimeOrError result)
            {
                if (result)
                {
                    auto newBuffer = newWindow.buffer();

                    if (not newBuffer)
                    {
                        return;
                    }

                    context.messageLine.info()
                        << "filtered " << newBuffer->lineCount() << " lines; took "
                        << (result.value() | utils::precision(3)) << " s";

                    context.mainLoop->executeTask(
                        [&newWindow, &context]
                        {
                            context.mainView.bufferLoaded(newWindow, context);
                        });
                }
                else if (context.running)
                {
                    context.messageLine.error() << "Error filtering buffer: " << result.error();
                    context.mainView.removeWindow(*newWindow.parent(), context);
                }
            });

        return true;
    }
}

}  // namespace core

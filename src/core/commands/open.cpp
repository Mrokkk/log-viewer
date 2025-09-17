#include "open.hpp"

#include "core/alias.hpp"
#include "core/buffer.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/interpreter.hpp"
#include "core/main_view.hpp"
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
            {Type::string, "path"}
        };
    };

    EXECUTOR()
    {
        auto path = *args[0].string();

        auto& newWindow = context.mainView.createWindow(path, MainView::Parent::root, context);

        auto newBuffer = newWindow.buffer();

        newBuffer->load(
            std::move(path),
            context,
            [&newWindow, &context](TimeOrError result)
            {
                if (context.running)
                {
                    context.mainView.bufferLoaded(std::move(result), newWindow, context);
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
    return interpreter::execute(buf.str(), context);
}

}  // namespace commands

}  // namespace core

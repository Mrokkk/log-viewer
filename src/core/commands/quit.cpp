#include "core/alias.hpp"
#include "core/context.hpp"
#include "core/interpreter/command.hpp"
#include "core/main_loop.hpp"
#include "core/main_view.hpp"

namespace core
{

DEFINE_COMMAND(quitall)
{
    HELP() = "quit the program";

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
        context.mainLoop->quit(context);
        return true;
    }
}

DEFINE_COMMAND(quit)
{
    HELP() = "close current window or quit the program";

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
        context.mainView.quitCurrentWindow(context);
        return true;
    }
}

DEFINE_ALIAS(q, quit);
DEFINE_ALIAS(qa, quitall);

}  // namespace core

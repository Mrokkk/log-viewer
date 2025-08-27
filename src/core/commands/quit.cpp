#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/user_interface.hpp"

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
        context.ui->quit(context);
        return true;
    }
}

DEFINE_ALIAS(qa, quitall);

}  // namespace core

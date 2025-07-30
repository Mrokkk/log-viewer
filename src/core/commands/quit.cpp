#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/user_interface.hpp"

namespace core
{

DEFINE_COMMAND(quit)
{
    HELP() = "quit the program";
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

DEFINE_ALIAS(q, quit);
DEFINE_ALIAS(qa, quit);

}  // namespace core

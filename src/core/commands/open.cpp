#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/operations.hpp"

#include "ui/ftxui.hpp"

namespace core
{

DEFINE_COMMAND(open)
{
    HELP() = "open a file";
    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "path")
        };
    };

    EXECUTOR()
    {
        return asyncViewLoader(args[0].string, context);
    }
}

DEFINE_ALIAS(e, open);

}  // namespace core

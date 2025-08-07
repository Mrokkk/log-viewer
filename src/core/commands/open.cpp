#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/operations.hpp"

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
        return asyncViewLoader(args[0].string, context);
    }
}

DEFINE_ALIAS(e, open);

}  // namespace core

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/operations.hpp"

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
        const auto& path = args[0];
        if (auto view = asyncViewLoader(path.string, context))
        {
            context.currentView = view;
            return true;
        }
        return false;
    }
}

DEFINE_ALIAS(e, open);

}  // namespace core

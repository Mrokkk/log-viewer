#include "core/command.hpp"
#include "core/variable.hpp"

namespace core
{

DEFINE_COMMAND(help)
{
    HELP() = "print help about command/variable";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "name")
        };
    };

    EXECUTOR()
    {
        const auto command = Commands::find(args[0].string);

        if (command)
        {
            *context.ui << info << command->name << ": " << command->help;
            return true;
        }

        const auto variable = Variables::find(args[0].string);

        if (not variable)
        {
            *context.ui << error << "No help entry for: " << args[0].string;
            return false;
        }

        *context.ui << info << variable->name << ": " << variable->help;
        return true;
    }
}

}  // namespace core

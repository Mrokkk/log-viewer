#include "core/command.hpp"
#include "core/message_line.hpp"
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
            context.messageLine << info << command->name << ": " << command->help;
            return true;
        }

        const auto variable = Variables::find(args[0].string);

        if (not variable)
        {
            context.messageLine << error << "No help entry for: " << args[0].string;
            return false;
        }

        context.messageLine << info << variable->name << ": " << variable->help;
        return true;
    }
}

}  // namespace core

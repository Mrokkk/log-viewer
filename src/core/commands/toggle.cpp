#include "core/command.hpp"
#include "core/message_line.hpp"
#include "core/variable.hpp"

namespace core
{

DEFINE_COMMAND(toggle)
{
    HELP() = "toggle the value of boolean variable";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "variable")
        };
    };

    EXECUTOR()
    {
        auto variable = Variables::find(args[0].string);

        if (not variable)
        {
            context.messageLine.error() << "Unknown variable: " << args[0].string;
            return false;
        }

        if (variable->type != Type::boolean)
        {
            context.messageLine.error() << "Not a boolean: " << args[0].string;
            return false;
        }

        if (variable->access != Variable::Access::readWrite)
        {
            context.messageLine.error() << "Not writable: " << args[0].string;
            return false;
        }

        auto value = variable->reader(context);

        bool modifiedValue = value.boolean;
        modifiedValue ^= true;
        Variable::Value modified(modifiedValue);

        variable->writer(modified, context);

        return true;
    }
}

}  // namespace core

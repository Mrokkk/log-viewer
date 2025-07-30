#include "core/command.hpp"
#include "core/variable.hpp"

namespace core
{

DEFINE_COMMAND(get)
{
    HELP() = "print value of variable";
    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "variable")
        };
    };

    EXECUTOR()
    {
        const auto variable = Variables::find(args[0].string);

        if (not variable)
        {
            *context.ui << error << "Unknown variable: " << args[0].string;
            return false;
        }

        *context.ui << info << VariableWithContext{*variable, context};

        return true;
    }
}

}  // namespace core

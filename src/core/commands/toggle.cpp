#include "core/interpreter/command.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/message_line.hpp"

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
            {Type::string, "variable"}
        };
    };

    EXECUTOR()
    {
        auto variableName = *args[0].string();
        auto variable = interpreter::Symbols::find(variableName);

        if (not variable)
        {
            context.messageLine.error() << "Unknown variable: " << variableName;
            return false;
        }

        auto boolean = variable->value().boolean();

        if (not boolean)
        {
            context.messageLine.error() << "Not a boolean: " << variableName;
            return false;
        }

        bool modifiedValue = boolean.value();
        modifiedValue ^= true;

        auto result = variable->assign(interpreter::Value(modifiedValue), context);

        if (not result)
        {
            context.messageLine.error() << "Cannot modify " << variableName << ": " << result.error();
            return false;
        }

        return true;
    }
}

}  // namespace core

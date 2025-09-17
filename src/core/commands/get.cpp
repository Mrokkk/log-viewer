#include "core/interpreter/command.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/message_line.hpp"

namespace core
{

DEFINE_COMMAND(get)
{
    HELP() = "print value of variable";

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
        const auto variableName = *args[0].string();
        const auto variable = interpreter::Symbols::find(variableName);

        if (not variable)
        {
            context.messageLine.error() << "Unknown variable: " << variableName;
            return false;
        }

        context.messageLine.info() << variable->value();

        return true;
    }
}

}  // namespace core

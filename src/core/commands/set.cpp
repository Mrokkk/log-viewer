#include "core/interpreter/command.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/message_line.hpp"

namespace core
{

DEFINE_COMMAND(set)
{
    HELP() = "set variable";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            {Type::string, "variable"},
            {Type::any, "value"}
        };
    };

    EXECUTOR()
    {
        const auto name = *args[0].string();
        const auto value = args[1];

        const auto variable = interpreter::Symbols::find(name);

        if (not variable)
        {
            interpreter::Symbols::addReadWrite(name, value);
            return true;
        }

        auto result = variable->assign(args[1], context);

        if (not result) [[unlikely]]
        {
            context.messageLine.error() << "Cannot set value: " << result.error();
            return false;
        }

        return true;
    }
}

}  // namespace core

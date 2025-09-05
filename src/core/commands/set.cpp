#include "core/command.hpp"
#include "core/message_line.hpp"
#include "core/variable.hpp"

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
            ARGUMENT(string, "variable"),
            ARGUMENT(any, "value")
        };
    };

    EXECUTOR()
    {
        const auto& name = args[0].string;

        const auto variable = Variables::find(name);

        if (not variable)
        {
            switch (args[1].type)
            {
                case Type::boolean:
                    Variables::addUserDefined(name, Type::boolean, args[1].boolean);
                    break;

                case Type::string:
                    Variables::addUserDefined(name, Type::string, args[1].string);
                    break;

                case Type::integer:
                    Variables::addUserDefined(name, Type::integer, args[1].integer);
                    break;

                default:
                    goto unknown;
            }
            return true;
        }

        if (variable->access != Variable::Access::readWrite)
        {
            context.messageLine.error() << "Not writable: " << args[0].string;
            return false;
        }

        switch (args[1].type)
        {
            case Type::boolean:
                variable->writer(Variable::Value(args[1].boolean), context);
                return true;

            case Type::string:
                variable->writer(Variable::Value(&args[1].string), context);
                return true;

            case Type::integer:
                variable->writer(Variable::Value(args[1].integer), context);
                return true;

            default:
                break;
        }

        unknown:
        {
            context.messageLine.error() << "Unknown type of value: " << args[1].type;
            return false;
        }
    }
}

}  // namespace core

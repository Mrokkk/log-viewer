#include <sstream>

#include "core/command.hpp"
#include "core/message_line.hpp"

namespace core
{

DEFINE_COMMAND(echo)
{
    HELP() = "print to message line";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            VARIADIC_ARGUMENT("args"),
        };
    };

    EXECUTOR()
    {
        std::stringstream ss;
        for (const auto& arg : args)
        {
            ss << arg.string << ' ';
        }
        context.messageLine << info << ss.rdbuf();
        return true;
    }
}

}  // namespace core

#include "core/command.hpp"

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
        *context.ui << info << ss.rdbuf();
        return true;
    }
}

}  // namespace core

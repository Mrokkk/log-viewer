#include "core/command.hpp"
#include "core/message_line.hpp"
#include "utils/buffer.hpp"

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
        utils::Buffer buf;
        for (const auto& arg : args)
        {
            buf << arg.string << ' ';
        }
        context.messageLine.info() << buf.view();
        return true;
    }
}

}  // namespace core

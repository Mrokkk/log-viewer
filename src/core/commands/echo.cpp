#include "core/interpreter/command.hpp"
#include "core/message_line.hpp"
#include "core/type.hpp"
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
            {Type::variadic, "args"},
        };
    };

    EXECUTOR()
    {
        utils::Buffer buf;
        for (const auto& arg : args)
        {
            buf << arg << ' ';
        }
        context.messageLine.info() << buf.view();
        return true;
    }
}

DEFINE_COMMAND(echoerr)
{
    HELP() = "print error to message line";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            {Type::variadic, "args"},
        };
    };

    EXECUTOR()
    {
        utils::Buffer buf;
        for (const auto& arg : args)
        {
            buf << arg << ' ';
        }
        context.messageLine.error() << buf.view();
        return true;
    }
}

}  // namespace core

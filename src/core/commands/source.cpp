#include "core/command.hpp"
#include "core/operations.hpp"
#include "utils/string.hpp"

namespace core
{

DEFINE_COMMAND(source)
{
    HELP() = "source given file";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "path")
        };
    }

    EXECUTOR()
    {
        auto code = args[0].string | utils::readText;
        return executeCode(code, context);
    }
}

}  // namespace core

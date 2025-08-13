#include "open.hpp"

#include <sstream>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/file_load.hpp"
#include "core/interpreter.hpp"

namespace core
{

DEFINE_COMMAND(open)
{
    HELP() = "open a file";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "path")
        };
    };

    EXECUTOR()
    {
        return asyncLoadFile(args[0].string, context);
    }
}

DEFINE_ALIAS(e, open);

namespace commands
{

bool open(const std::string& path, Context& context)
{
    std::stringstream ss;
    ss << open::Command::name << " \"" << path << "\"";
    return executeCode(ss.str(), context);
}

}  // namespace commands

}  // namespace core

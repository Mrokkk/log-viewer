#include "source.hpp"

#include "core/command.hpp"
#include "core/interpreter.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"
#include "utils/units.hpp"

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
        auto code = args[0].string | utils::readText(1_MiB);
        return executeCode(code, context);
    }
}

namespace commands
{

bool source(const std::string& filename, Context& context)
{
    utils::Buffer buf;
    buf << source::Command::name << " \"" << filename << '\"';
    return executeCode(buf.str(), context);
}

}  // namespace commands

}  // namespace core

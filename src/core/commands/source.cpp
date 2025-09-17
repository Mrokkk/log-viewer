#include "source.hpp"

#include "core/interpreter/command.hpp"
#include "core/interpreter/interpreter.hpp"
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
            {Type::string, "path"}
        };
    }

    EXECUTOR()
    {
        auto code = *args[0].string() | utils::readText(1_MiB);
        return interpreter::execute(code, context);
    }
}

namespace commands
{

bool source(const std::string& filename, Context& context)
{
    utils::Buffer buf;
    buf << source::Command::name << " \"" << filename << '\"';
    return interpreter::execute(buf.str(), context);
}

}  // namespace commands

}  // namespace core

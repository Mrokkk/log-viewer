#include "core/command.hpp"
#include "core/input_mapping.hpp"

namespace core
{

DEFINE_COMMAND(map)
{
    HELP() = "map keys to the command";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            ARGUMENT(string, "keys"),
            ARGUMENT(string, "command")
        };
    }

    EXECUTOR()
    {
        auto keys = args[0].string;
        const auto& command = args[1].string;
        return addInputMapping(std::move(keys), command, force, context);
    }
}

}  // namespace core

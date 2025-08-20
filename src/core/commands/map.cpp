#include "core/command.hpp"
#include "core/input.hpp"

namespace core
{

#define MAP_COMMAND(PREFIX, MODE) \
    DEFINE_COMMAND(PREFIX##map) \
    { \
        HELP() = "map key sequence {lhs} to the {rhs} in " #MODE " mode"; \
        FLAGS() { return {}; } \
        ARGUMENTS() \
        { \
            return { \
                ARGUMENT(string, "lhs"), \
                ARGUMENT(string, "rhs") \
            }; \
        } \
        EXECUTOR() \
        { \
            const auto& lhs = args[0].string; \
            const auto& rhs = args[1].string; \
            auto inputFlags = InputMappingFlags::MODE | (force ? InputMappingFlags::force : InputMappingFlags::none); \
            return addInputMapping(lhs, rhs, inputFlags, context); \
        } \
    }

MAP_COMMAND(n, normal)
MAP_COMMAND(v, visual)
MAP_COMMAND(c, command)

}  // namespace core

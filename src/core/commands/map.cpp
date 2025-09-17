#include "core/interpreter/command.hpp"
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
                {Type::string, "lhs"}, \
                {Type::string, "rhs"} \
            }; \
        } \
        EXECUTOR() \
        { \
            const auto lhs = *args[0].stringView(); \
            const auto rhs = *args[1].stringView(); \
            InputMappingFlags inputFlags; \
            if (force) \
            { \
                inputFlags |= InputMappingFlags::force; \
            } \
            inputFlags |= InputMappingFlags::MODE; \
            return addInputMapping(lhs, rhs, inputFlags, context); \
        } \
    }

MAP_COMMAND(n, normal)
MAP_COMMAND(v, visual)
MAP_COMMAND(c, command)

}  // namespace core

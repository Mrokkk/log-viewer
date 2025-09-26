#include "core/input.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/value.hpp"

namespace core
{

static std::string getHelp(const interpreter::Values& args)
{
    if (args.size() > 2)
    {
        auto value = args[2].string();
        if (value)
        {
            return *value;
        }
    }
    return "User defined mapping";
}

static bool map(const interpreter::Values& args, InputMappingFlags mode, bool force, Context& context)
{
    const auto lhs = *args[0].stringView();
    const auto rhs = *args[1].stringView();
    InputMappingFlags inputFlags = mode;
    if (force)
    {
        inputFlags |= InputMappingFlags::force;
    }
    return addInputMapping(lhs, rhs, inputFlags, getHelp(args), context);
}

#define MAP_COMMAND(PREFIX, MODE) \
    DEFINE_COMMAND(PREFIX##map) \
    { \
        HELP() = "map key sequence {lhs} to the {rhs} in " #MODE " mode"; \
        FLAGS() { return {}; } \
        ARGUMENTS() \
        { \
            return { \
                {Type::string, "lhs"}, \
                {Type::string, "rhs"}, \
                {Type::variadic, "name"}, \
            }; \
        } \
        EXECUTOR() \
        { \
            return map(args, InputMappingFlags::MODE, force, context); \
        } \
    }

MAP_COMMAND(n, normal)
MAP_COMMAND(v, visual)
MAP_COMMAND(c, command)

}  // namespace core

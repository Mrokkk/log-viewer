#include "input.hpp"

#include "core/context.hpp"
#include "core/input_mapping.hpp"
#include "core/interpreter.hpp"

namespace core
{

bool registerKeyPress(char c, Context& context)
{
    auto& inputMappings = InputMappings::map();
    auto& inputMappingSet = InputMappings::set();

    if (c == '\e')
    {
        context.inputState.state.clear();
        return true;
    }

    context.inputState.state += c;

    if (not inputMappingSet.contains(context.inputState.state))
    {
        context.inputState.state.clear();
        return false;
    }

    auto inputMappingIt = inputMappings.find(context.inputState.state);

    if (inputMappingIt != inputMappings.end())
    {
        executeCode(inputMappingIt->second.command, context);
        context.inputState.state.clear();
    }

    return true;
}

}  // namespace core

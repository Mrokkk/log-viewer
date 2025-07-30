#include "core/command.hpp"

#include <algorithm>
#include <iostream>

namespace core
{

CommandsMap& Commands::map()
{
    static CommandsMap map;
    return map;
}

Command* Commands::find(const std::string& name)
{
    auto& commands = Commands::map();
    const auto commandIt = commands.find(name);

    return commandIt != commands.end()
        ? &commandIt->second
        : nullptr;
}

void Commands::$register(Command command)
{
    auto& commands = Commands::map();

    const auto& commandArgs = command.arguments.get();

    const auto variadicArgument = std::ranges::find_if(
        commandArgs,
        [](const auto& e){ return e.type == Type::variadic; });

    if (variadicArgument != commandArgs.end() and variadicArgument != commandArgs.end() - 1) [[unlikely]]
    {
        std::cerr << __func__ << ": " << command.name << ": variadic argument can appear only as last one\n";
        std::abort();
    }

    auto result = commands.emplace(std::make_pair(command.name, std::move(command)));

    if (not result.second) [[unlikely]]
    {
        std::cerr << __func__ << ": " << command.name << ": already defined\n";
        std::abort();
    }
}

}  // namespace core

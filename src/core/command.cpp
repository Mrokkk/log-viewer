#include "core/command.hpp"

#include <algorithm>
#include <cstdio>
#include <string_view>

namespace core
{

CommandArguments::CommandArguments() = default;

CommandArguments::CommandArguments(std::initializer_list<ArgumentSignature> n)
    : types_(std::move(n))
{
}

const std::vector<CommandArguments::ArgumentSignature>& CommandArguments::get() const
{
    return types_;
}

CommandFlags::CommandFlags() = default;

CommandFlags::CommandFlags(std::initializer_list<std::string> n)
    : flags_(std::move(n))
{
}

const std::flat_set<std::string>& CommandFlags::get() const
{
    return flags_;
}

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
        std::fprintf(stderr, "%s: %s: variadic argument can appear only as last one\n", __func__, command.name.data());
        std::abort();
    }

    auto result = commands.emplace(std::make_pair(command.name, std::move(command)));

    if (not result.second) [[unlikely]]
    {
        std::fprintf(stderr, "%s: %s: already defined\n", __func__, command.name.data());
        std::abort();
    }
}

}  // namespace core

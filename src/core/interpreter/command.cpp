#include "command.hpp"

#include <algorithm>
#include <cstdio>
#include <flat_map>
#include <string_view>

namespace core::interpreter
{

using CommandsMap = std::flat_map<std::string_view, Command>;

static CommandsMap& map()
{
    static CommandsMap map;
    return map;
}

CommandArguments::CommandArguments() = default;

CommandArguments::CommandArguments(std::initializer_list<ArgumentSignature> n)
    : mTypes(std::move(n))
{
}

CommandArguments::~CommandArguments() = default;

const std::vector<CommandArguments::ArgumentSignature>& CommandArguments::get() const
{
    return mTypes;
}

CommandFlags::CommandFlags() = default;

CommandFlags::~CommandFlags() = default;

CommandFlags::CommandFlags(std::initializer_list<FlagSignature>&& n)
    : mFlags(std::move(n))
{
}

const std::vector<CommandFlags::FlagSignature>& CommandFlags::get() const
{
    return mFlags;
}

Command* Commands::find(const std::string& name)
{
    auto& commands = map();
    const auto commandIt = commands.find(name);

    return commandIt != commands.end()
        ? &commandIt->second
        : nullptr;
}

static void validate(const Command& command)
{
    const auto& commandArgs = command.arguments.get();

    const auto variadicArgument = std::ranges::find_if(
        commandArgs,
        [](const auto& e){ return e.type == Type::variadic; });

    if (variadicArgument != commandArgs.end() and variadicArgument != commandArgs.end() - 1) [[unlikely]]
    {
        std::fprintf(stderr, "%s: %s: variadic argument can appear only as last one\n", __func__, command.name.data());
        std::abort();
    }
}

void Commands::$register(Command command)
{
    validate(command);

    auto& commands = map();

    auto result = commands.emplace(std::make_pair(command.name, std::move(command)));

    if (not result.second) [[unlikely]]
    {
        std::fprintf(stderr, "%s: %s: already defined\n", __func__, command.name.data());
        std::abort();
    }
}

void Commands::forEach(std::function<void(const Command&)> callback)
{
    for (const auto& [_, command] : map())
    {
        callback(command);
    }
}

}  // namespace core::interpreter

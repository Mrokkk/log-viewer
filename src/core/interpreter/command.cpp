#include "command.hpp"

#include <algorithm>
#include <cstdio>
#include <string_view>

#include "utils/function_ref.hpp"
#include "utils/hash_map.hpp"

namespace core::interpreter
{

using CommandsMap = utils::HashMap<std::string_view, Command>;

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
    const auto node = commands.find(name);

    return node
        ? &node->second
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

    if (commands.find(command.name)) [[unlikely]]
    {
        std::fprintf(stderr, "%s: %s: already defined\n", __func__, command.name.data());
        std::abort();
    }

    commands.insert(command.name, std::move(command));
}

void Commands::forEach(utils::FunctionRef<void(const Command&)> callback)
{
    map().forEach(
        [&callback](const auto& node)
        {
            callback(node.second);
        });
}

}  // namespace core::interpreter

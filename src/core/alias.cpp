#include "alias.hpp"

#include <flat_map>

namespace core
{

using AliasesMap = std::flat_map<std::string_view, Alias>;

static AliasesMap& map()
{
    static AliasesMap map;
    return map;
}

void Aliases::$register(Alias alias)
{
    map()[alias.name] = std::move(alias);
}

Alias* Aliases::find(const std::string_view& name)
{
    auto aliasIt = map().find(name);

    if (aliasIt == map().end()) [[unlikely]]
    {
        return nullptr;
    }

    return &aliasIt->second;
}

void Aliases::forEach(std::function<void(const Alias&)> callback)
{
    for (const auto& [_, alias] : map())
    {
        callback(alias);
    }
}

}  // namespace core

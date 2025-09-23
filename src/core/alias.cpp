#include "alias.hpp"

#include "utils/function_ref.hpp"
#include "utils/hash_map.hpp"

namespace core
{

using AliasesMap = utils::HashMap<std::string_view, Alias>;

static AliasesMap& map()
{
    static AliasesMap map;
    return map;
}

void Aliases::$register(Alias alias)
{
    map().insert(alias.name, std::move(alias));
}

Alias* Aliases::find(const std::string_view& name)
{
    auto node = map().find(name);

    if (not node) [[unlikely]]
    {
        return nullptr;
    }

    return &node->second;
}

void Aliases::forEach(utils::FunctionRef<void(const Alias&)> callback)
{
    map().forEach(
        [&callback](const auto& node)
        {
            callback(node.second);
        });
}

}  // namespace core

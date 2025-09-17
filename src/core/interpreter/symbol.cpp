#include "symbol.hpp"

#include <expected>
#include <utility>

#include "core/interpreter/symbols_map.hpp"
#include "utils/buffer.hpp"
#include "utils/unique_ptr.hpp"

namespace core::interpreter
{

OpResult Symbol::assign(Value newValue, Context&)
{
    if (mAccess == Access::readOnly)
    {
        return std::unexpected("Not writable");
    }

    return value().assign(newValue);
}

static SymbolsMap& map()
{
    static SymbolsMap map;
    return map;
}

const SymbolsMap& symbolsMap()
{
    return map();
}

Symbol* Symbols::find(const std::string& name)
{
    const auto& symbols = map();

    const auto variableIt = symbols.find(name);

    return variableIt != symbols.end()
        ? &variableIt->second.get()
        : nullptr;
}

void Symbols::addReadWrite(std::string name, Value value)
{
    map().emplace(std::move(name), utils::makeUnique<Symbol>(Symbol::Access::readWrite, value));
}

void Symbols::addReadOnly(std::string name, Value value)
{
    map().emplace(std::move(name), utils::makeUnique<Symbol>(Symbol::Access::readOnly, value));
}

void Symbols::add(std::string name, Symbol& symbol)
{
    map().emplace(std::move(name), &symbol);
}

utils::Buffer& operator<<(utils::Buffer& buf, const Symbol& symbol)
{
    return buf << symbol.value();
}

}  // namespace core::interpreter

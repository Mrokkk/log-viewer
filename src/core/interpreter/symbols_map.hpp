#pragma once

#include <flat_map>
#include <string>

#include "core/interpreter/symbol.hpp"
#include "utils/unique_ptr.hpp"

namespace core::interpreter
{

struct SymbolNode final
{
    constexpr SymbolNode(utils::UniquePtr<Symbol> symbol)
        : mOwned(true)
        , mSymbol(symbol.release())
    {
    }

    constexpr SymbolNode(Symbol* symbol)
        : mOwned(false)
        , mSymbol(symbol)
    {
    }

    constexpr SymbolNode(SymbolNode&& other)
        : mOwned(other.mOwned)
        , mSymbol(other.mSymbol)
    {
        other.mOwned = false;
        other.mSymbol = nullptr;
    }

    constexpr SymbolNode& operator=(SymbolNode&& other)
    {
        mOwned = other.mOwned;
        mSymbol = other.mSymbol;
        other.mOwned = false;
        other.mSymbol = nullptr;
        return *this;
    }

    constexpr ~SymbolNode()
    {
        if (mOwned)
        {
            delete mSymbol;
        }
    }

    constexpr inline auto operator->() const { return mSymbol; }
    constexpr inline auto& operator*() const { return *mSymbol; }
    constexpr inline auto& get()             { return *mSymbol; }
    constexpr inline auto& get()       const { return *mSymbol; }

private:
    bool    mOwned;
    Symbol* mSymbol;
};

using SymbolsMap = std::flat_map<std::string, SymbolNode>;

const SymbolsMap& symbolsMap();

}  // namespace core::interpreter

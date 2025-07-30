#pragma once

#include <flat_map>
#include <string_view>

namespace core
{

struct Alias
{
    std::string_view name;
    std::string_view command;
};

using AliasesMap = std::flat_map<std::string_view, Alias>;

struct Aliases final
{
    Aliases() = delete;
    static AliasesMap& map();
};

#define DEFINE_ALIAS(NAME, COMMAND) \
    static void __attribute__((constructor)) alias_##NAME##_init() \
    { \
        ::core::Aliases::map()[#NAME] = ::core::Alias{.name = #NAME, .command = #COMMAND}; \
    }

}  // namespace core

#pragma once

#include <string_view>

#include "utils/function_ref.hpp"

namespace core
{

struct Alias
{
    std::string_view name;
    std::string_view command;
};

struct Aliases final
{
    Aliases() = delete;
    static void $register(Alias alias);
    static Alias* find(const std::string_view& name);
    static void forEach(utils::FunctionRef<void(const Alias&)> callback);
};

#define DEFINE_ALIAS(NAME, COMMAND) \
    static void __attribute__((constructor)) alias_##NAME##_init() \
    { \
        ::core::Aliases::$register( \
            ::core::Alias{ \
                .name = #NAME, \
                .command = #COMMAND \
            }); \
    }

}  // namespace core

#pragma once

#include <flat_map>
#include <flat_set>
#include <string>
#include <string_view>

#include "core/context.hpp"
#include "core/type.hpp"
#include "utils/noncopyable.hpp"

namespace core
{

struct Argument
{
    Type        type;
    union
    {
        long    integer;
        bool    boolean;
    };
    std::string string;
};

using Arguments = std::vector<Argument>;
using Flags = std::flat_set<std::string>;

using CommandHandler = bool(*)(const Arguments& args, const Flags& flags, bool force, Context& context);

struct CommandArguments final
{
    struct ArgumentSignature
    {
        Type        type;
        const char* name;
    };

    CommandArguments() = default;

    CommandArguments(std::initializer_list<ArgumentSignature> n)
        : types_(std::move(n))
    {
    }

    const std::vector<ArgumentSignature>& get() const
    {
        return types_;
    }

private:
    std::vector<ArgumentSignature> types_;
};

struct CommandFlags final
{
    CommandFlags() = default;

    CommandFlags(std::initializer_list<std::string> n)
        : flags_(std::move(n))
    {
    }

    const std::flat_set<std::string>& get() const
    {
        return flags_;
    }

private:
    std::flat_set<std::string> flags_;
};

struct Command final : utils::Noncopyable
{
    std::string_view name;
    CommandArguments arguments;
    CommandFlags     flags;
    CommandHandler   handler;
    const char*      help;
};

using CommandsMap = std::flat_map<std::string_view, Command>;

struct Commands final
{
    Commands() = delete;
    static CommandsMap& map();
    static Command* find(const std::string& name);
    static void $register(Command command);
};

#define EXECUTOR() \
    bool Command::execute( \
        [[maybe_unused]] const ::core::Arguments& args, \
        [[maybe_unused]] const ::core::Flags& flags, \
        [[maybe_unused]] bool force, \
        [[maybe_unused]] ::core::Context& context)

#define HELP() \
    const char* Command::help

#define ARGUMENTS() \
    ::core::CommandArguments Command::arguments()

#define FLAGS() \
    ::core::CommandFlags Command::flags()

#define ARGUMENT(TYPE, NAME) \
    ::core::CommandArguments::ArgumentSignature{ \
        .type = ::core::Type::TYPE, \
        .name = NAME \
    }

#define VARIADIC_ARGUMENT(NAME) \
    ::core::CommandArguments::ArgumentSignature{ \
        .type = ::core::Type::variadic, \
        .name = NAME \
    }

#define DEFINE_COMMAND(NAME) \
    namespace NAME \
    { \
    struct Command \
    { \
        static bool execute(const ::core::Arguments& args, const ::core::Flags& flags, bool force, ::core::Context& context); \
        static void init() \
        { \
            ::core::Commands::$register( \
                ::core::Command{ \
                    .name = #NAME, \
                    .arguments = Command::arguments(), \
                    .flags = Command::flags(), \
                    .handler = &Command::execute, \
                    .help = Command::help \
                }); \
        } \
        static const char* help; \
        static ::core::CommandArguments arguments(); \
        static ::core::CommandFlags flags(); \
        static inline bool registered = (Command::init(), true); \
    }; \
    } \
    namespace NAME

}  // namespace core

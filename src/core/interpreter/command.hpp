#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "core/context.hpp"
#include "core/interpreter/value.hpp"
#include "core/type.hpp"
#include "utils/bitflag.hpp"
#include "utils/enum_traits.hpp"
#include "utils/noncopyable.hpp"

namespace core::interpreter
{

using CommandHandler = bool(*)(const Values& args, const int flagsMask, bool force, Context& context);

struct CommandArguments final
{
    struct ArgumentSignature
    {
        Type        type;
        const char* name;
    };

    CommandArguments();
    CommandArguments(std::initializer_list<ArgumentSignature> n);
    ~CommandArguments();

    const std::vector<ArgumentSignature>& get() const;

private:
    std::vector<ArgumentSignature> mTypes;
};

struct CommandFlags final
{
    struct FlagSignature
    {
        FlagSignature()
            : mask(0)
        {
        }

        template <typename T>
        requires utils::Enum<T>
        constexpr FlagSignature(std::string_view s, T flag)
            : name(s)
            , mask(utils::bitMask<int>(flag))
        {
        }

        std::string_view name;
        long             mask;
    };

    CommandFlags();
    ~CommandFlags();

    CommandFlags(std::initializer_list<FlagSignature>&& n);

    const std::vector<FlagSignature>& get() const;

private:
    std::vector<FlagSignature> mFlags;
};

struct Command final : utils::NonCopyable
{
    std::string_view name;
    CommandArguments arguments;
    CommandFlags     flags;
    CommandHandler   handler;
    const char*      help;
};

struct Commands final
{
    Commands() = delete;
    static Command* find(const std::string& name);
    static void $register(Command command);
    static void forEach(std::function<void(const Command&)> callback);
};

#define EXECUTOR() \
    bool Command::execute( \
        [[maybe_unused]] const ::core::interpreter::Values& args, \
        [[maybe_unused]] const int flagsMask, \
        [[maybe_unused]] bool force, \
        [[maybe_unused]] ::core::Context& context)

#define HELP() \
    const char* Command::help

#define ARGUMENTS() \
    ::core::interpreter::CommandArguments Command::arguments()

#define FLAGS() \
    ::core::interpreter::CommandFlags Command::flags()

#define DEFINE_COMMAND(NAME) \
    namespace NAME \
    { \
    struct Command \
    { \
        static bool execute(const ::core::interpreter::Values& args, const int flags, bool force, ::core::Context& context); \
        static void init() \
        { \
            ::core::interpreter::Commands::$register( \
                ::core::interpreter::Command{ \
                    .name = #NAME, \
                    .arguments = Command::arguments(), \
                    .flags = Command::flags(), \
                    .handler = &Command::execute, \
                    .help = Command::help \
                }); \
        } \
        static const char* help; \
        static ::core::interpreter::CommandArguments arguments(); \
        static ::core::interpreter::CommandFlags flags(); \
        static inline const char* name = #NAME; \
        static inline bool registered = (Command::init(), true); \
    }; \
    } \
    namespace NAME

}  // namespace core::interpreter

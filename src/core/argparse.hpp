#pragma once

#include <expected>
#include <functional>
#include <list>
#include <string_view>

#include "core/type.hpp"
#include "utils/immobile.hpp"
#include "utils/noncopyable.hpp"

namespace core
{

struct Option
{
    enum Variant
    {
        positional,
        additional,
    };

    struct Value : utils::Noncopyable
    {
        Value();
        explicit Value(int value);
        explicit Value(std::string_view value);
        explicit Value(bool value);

        Type                 type;
        union
        {
            void*            ptr;
            long             integer;
            std::string_view string;
            bool             boolean;
        };
    };

    using OnMatch = std::function<void(const Value&)>;

    ~Option();

    core::Type       type;
    Variant          variant;
    std::string_view longName;
    char             shortName;
    std::string_view help;
    Value*           value;
    OnMatch          onMatch;
};

using CommandLineOptionsList = std::list<Option>;

struct CommandLineOptions final : utils::Immobile
{
    static CommandLineOptionsList& list();
};

struct CommandLineOption final : utils::Immobile
{
    CommandLineOption(Option option);
    operator bool() const;
    Option::Value* operator->();
    Option::Value& operator*();

private:
    Option& option_;
};

std::expected<int, std::string> parseArgs(int argc, char* const* argv);

}  // namespace core

#include "argparse.hpp"

#include <cassert>
#include <flat_map>
#include <getopt.h>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string_view>
#include <vector>

#include "core/type.hpp"
#include "utils/buffer.hpp"

namespace core
{

[[noreturn]] static void printHelp(const Option::Value&)
{
    auto& registeredOptions = CommandLineOptions::list();

    std::vector<const Option*> positionalOptions;

    size_t columnWidth{0};

    for (const auto& option : registeredOptions)
    {
        size_t optionWidth{10};
        if (not option.longName.empty())
        {
            optionWidth += 2 + option.longName.length();
        }
        if (optionWidth > columnWidth)
        {
            columnWidth = optionWidth;
        }
        if (option.variant == Option::positional)
        {
            positionalOptions.push_back(&option);
        }
    }

    std::cerr << "Usage: " << program_invocation_short_name << " [option]...";

    for (const auto positionalOption : positionalOptions)
    {
        std::cerr << " [" << positionalOption->longName << ']';
    }

    std::cerr << "\n\n";

    std::cerr << "Options:\n\n";

    for (const auto& option : registeredOptions)
    {
        if (option.variant == Option::positional)
        {
            continue;
        }

        utils::Buffer buf;

        buf << "  ";

        if (option.shortName)
        {
            buf << "-" << option.shortName;
            if (not option.longName.empty())
            {
                buf << ", ";
            }
            else
            {
                buf << "  ";
            }
        }
        else
        {
            buf << "    ";
        }
        if (not option.longName.empty())
        {
            buf << "--" << option.longName;
        }

        std::cerr << std::left << std::setw(columnWidth) << buf.str() << std::setw(0) << option.help << '\n';
    }

    std::cerr << std::endl;

    exit(0);
}

static CommandLineOption help(
    {
        .type = core::Type::boolean,
        .variant = Option::Variant::additional,
        .longName = "help",
        .help = "show help",
        .onMatch = &printHelp
    });

#define INTERNAL_ERROR(...) \
    "Internal error: " << __VA_ARGS__

static void saveValue(Option& option)
{
    switch (option.type)
    {
        case Type::boolean:
            assert(optarg == nullptr);
            option.value = new Option::Value(true);
            break;
        case Type::string:
            assert(optarg != nullptr);
            option.value = new Option::Value(std::string_view(optarg));
            break;
        default:
            assert(false);
    }

    if (option.onMatch)
    {
        option.onMatch(*option.value);
    }
}

std::expected<int, std::string> parseArgs(int argc, char* const* argv)
{
    utils::Buffer error;
    std::string shortopts = "-";
    std::vector<option> longopts;

    std::flat_map<std::string_view, Option*> longNameToOption;
    std::flat_map<char, Option*> shortNameToOption;
    Option* positionalOption = nullptr;

    auto& registeredOptions = CommandLineOptions::list();

    for (auto& option : registeredOptions)
    {
        if (option.variant == Option::Variant::additional)
        {
            longopts.emplace_back(::option{
                .name = option.longName.data(),
                .has_arg = option.type == core::Type::integer or option.type == core::Type::string
                    ? required_argument
                    : no_argument,
                .flag = nullptr,
                .val = 0,
            });

            auto result = longNameToOption.emplace(std::make_pair(option.longName, &option));

            if (not result.second)
            {
                error << INTERNAL_ERROR(option.longName << ": long name already registered");
                return std::unexpected(error.str());
            }

            if (option.shortName)
            {
                auto result = shortNameToOption.emplace(std::make_pair(option.shortName, &option));

                if (not result.second)
                {
                    error << INTERNAL_ERROR(option.shortName << ": short name already registered");
                    return std::unexpected(error.str());
                }

                shortopts += option.shortName;
                if (option.type != core::Type::boolean)
                {
                    shortopts += ":";
                }
            }
        }
        else
        {
            positionalOption = &option;
        }
    }

    longopts.emplace_back(option{0, 0, 0, 0});

    while (1)
    {
        int optindex = -1;
        const int res = getopt_long_only(argc, argv, shortopts.c_str(), longopts.data(), &optindex);
        switch (res)
        {
            case 0:
            {
                auto optionIt = longNameToOption.find(longopts[optindex].name);

                if (optionIt == longNameToOption.end())
                {
                    error << INTERNAL_ERROR("unknown option");
                    return std::unexpected(error.str());
                }

                saveValue(*optionIt->second);
                break;
            }

            case 1:
            {
                if (not positionalOption)
                {
                    error << INTERNAL_ERROR("unknown option " << optarg);
                    return std::unexpected(error.str());
                }

                auto option = positionalOption;

                switch (option->type)
                {
                    case Type::string:
                        assert(optarg != nullptr);
                        option->value = new Option::Value(std::string_view(optarg));
                        break;
                    default:
                        assert(false);
                }

                if (option->onMatch)
                {
                    option->onMatch(*option->value);
                }

                break;
            }

            case '?':
                return std::unexpected("");

            case -1:
                return 0;

            default:
            {
                auto optionIt = shortNameToOption.find(res);

                if (optionIt != shortNameToOption.end())
                {
                    saveValue(*optionIt->second);
                }
                break;
            }
        }
    }

    return 0;
}

Option::~Option()
{
    if (value)
    {
        delete value;
    }
}

Option::Value::Value()
    : type(Type::null)
    , ptr(nullptr)
{
}

Option::Value::Value(int value)
    : type(Type::integer)
    , integer(value)
{
}

Option::Value::Value(std::string_view value)
    : type(Type::string)
    , string(value)
{
}

Option::Value::Value(bool value)
    : type(Type::boolean)
    , boolean(value)
{
}

CommandLineOptionsList& CommandLineOptions::list()
{
    static CommandLineOptionsList list;
    return list;
}

CommandLineOption::CommandLineOption(Option option)
    : mOption(CommandLineOptions::list().emplace_back(option))
{
}

CommandLineOption::operator bool() const
{
    return mOption.value != nullptr;
}

const Option::Value* CommandLineOption::operator->() const
{
    return mOption.value;
}

const Option::Value& CommandLineOption::operator*() const
{
    return *mOption.value;
}

}  // namespace core

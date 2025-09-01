#include "interpreter.hpp"

#include <algorithm>
#include <expected>
#include <span>
#include <string>
#include <string_view>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/lexer.hpp"
#include "core/message_line.hpp"
#include "core/type.hpp"
#include "core/user_interface.hpp"
#include "core/variable.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

namespace core
{

using TokensSpan = std::span<const Token>;

std::expected<Variable::Value, bool> resolveReference(const std::string& name, Context& context)
{
    const auto variable = Variables::find(name);

    if (not variable)
    {
        context.messageLine.error() << "No such variable: " << name;
        return std::unexpected(false);
    }

    return variable->reader(context);
}

std::expected<std::string, bool> resolveCurrentPath(Context& context)
{
    auto path = Variables::find("path");

    if (not path)
    {
        context.messageLine.error() << "path variable does not exist";
        return std::unexpected(false);
    }

    auto value = path->reader(context);

    if (not value.string)
    {
        context.messageLine.error() << "path not set";
        return std::unexpected(false);
    }

    return *value.string;
}

static bool executeShellCommand(const TokensSpan& tokens, Context& context)
{
    const char* start = nullptr;
    const char* end = nullptr;
    utils::Buffer command;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (tokens[i].type == Token::Type::dollar and i + 1 < tokens.size() and tokens[i + 1].type == Token::Type::identifier)
        {
            if (start)
            {
                end = tokens[i].value.data();
                command << std::string_view(start, end);
            }

            ++i;
            const auto value = resolveReference(std::string(tokens[i].value), context);

            if (not value)
            {
                return false;
            }

            command << getValueString(value.value());

            start = nullptr;
        }
        else if (tokens[i].type == Token::Type::percent)
        {
            if (start)
            {
                end = tokens[i].value.data();
                command << std::string_view(start, end);
            }

            const auto value = resolveCurrentPath(context);

            if (not value)
            {
                return false;
            }

            command << value.value();
            start = nullptr;
        }
        else if (tokens[i].type == Token::Type::stringLiteral)
        {
            if (start)
            {
                end = tokens[i].value.data() - 1;
                command << std::string_view(start, end);
            }

            command << tokens[i].value;

            start = tokens[i].value.data() + tokens[i].value.size() + 1;
        }
        else if (not start)
        {
            start = tokens[i].value.data();
            end = start + tokens[i].value.size();
        }
        else
        {
            end = tokens[i].value.data() + tokens[i].value.size();
        }
    }

    if (start and end)
    {
        command << std::string_view(start, end);
    }

    context.ui->executeShell(command.str());

    return true;
}

static bool executeGoToCommand(const TokensSpan& tokens, Context& context)
{
    const auto lineNumber = tokens[0].value | utils::to<long>;
    context.ui->scrollTo(static_cast<Scroll>(lineNumber), context);
    return true;
}

static bool executeCommand(const TokensSpan& tokens, Context& context)
{
    std::string commandName(tokens[0].value.begin(), tokens[0].value.end());

    const auto& aliases = Aliases::map();

    const auto alias = aliases.find(commandName);

    if (alias != aliases.end())
    {
        commandName = alias->second.command;
    }

    bool force{false};
    Arguments args;
    Flags flags;

    for (size_t i = 1; i < tokens.size(); ++i)
    {
        if (tokens[i].type == Token::Type::exclamation)
        {
            if (i > 1)
            {
                context.messageLine.error() << "! in unexpected place";
                return false;
            }
            force = true;
            continue;
        }
        switch (tokens[i].type)
        {
            case Token::Type::sub:
                switch (tokens[i + 1].type)
                {
                    case Token::Type::intLiteral:
                        args.emplace_back(
                            Argument{
                                .type = Type::integer,
                                .integer = -1 * (tokens[i + 1].value | utils::to<long>),
                                .string{tokens[i].value.begin(), tokens[i + 1].value.end()},
                            });
                        ++i;
                        break;
                    case Token::Type::identifier:
                        flags.insert(std::string(tokens[i + 1].value));
                        ++i;
                        break;
                    default:
                        goto error;
                }
                break;

            case Token::Type::add:
                if (tokens[i + 1].type != Token::Type::intLiteral)
                {
                    goto error;
                }
                ++i;
                [[fallthrough]];

            case Token::Type::intLiteral:
                args.emplace_back(
                    Argument{
                        .type = Type::integer,
                        .integer = tokens[i].value | utils::to<long>,
                        .string{tokens[i].value.begin(), tokens[i].value.end()},
                    });
                break;

            case Token::Type::booleanLiteral:
                args.emplace_back(
                    Argument{
                        .type = Type::boolean,
                        .integer = tokens[i].value == "true",
                        .string{tokens[i].value.begin(), tokens[i].value.end()},
                    });
                break;

            case Token::Type::identifier:
            case Token::Type::stringLiteral:
                args.emplace_back(
                    Argument{
                        .type = Type::string,
                        .string{tokens[i].value.begin(), tokens[i].value.end()},
                    });
                break;

            case Token::Type::percent:
            {
                auto maybePath = resolveCurrentPath(context);

                if (not maybePath)
                {
                    return false;
                }

                args.emplace_back(
                    Argument{
                        .type = Type::string,
                        .string = std::move(maybePath.value()),
                    });
                break;
            }

            case Token::Type::dollar:
            {
                if (tokens[i + 1].type != Token::Type::identifier)
                {
                    goto error;
                }
                ++i;
                auto maybeValue = resolveReference(std::string(tokens[i].value), context);

                if (not maybeValue)
                {
                    return false;
                }

                auto& value = maybeValue.value();

                auto argument = Argument{
                    .type = value.type,
                    .string{getValueString(value)},
                };

                switch (value.type)
                {
                    case Type::boolean:
                        argument.boolean = value.boolean;
                        break;
                    case Type::integer:
                        argument.integer = value.integer;
                        break;
                    default:
                        break;
                }

                args.emplace_back(std::move(argument));

                break;
            }

            case Token::Type::whitespace:
            case Token::Type::comment:
                continue;

            error:
            default:
                context.messageLine.error() << "Unexpected token: " << tokens[i];
                return false;
        }
    }

    const auto command = Commands::find(commandName);

    if (not command)
    {
        context.messageLine.error() << "Unknown command: " << commandName;
        return false;
    }

    const auto& commandArgs = command->arguments.get();

    const bool hasVariadicArgument = commandArgs.size() and commandArgs.back().type == Type::variadic;
    const auto commandArgsCount = hasVariadicArgument ? commandArgs.size() - 1 : commandArgs.size();

    if (commandArgsCount != args.size())
    {
        if (not (hasVariadicArgument and args.size() > commandArgsCount))
        {
            context.messageLine.error() << "Invalid number of arguments passed to " << commandName
                        << "; expected " << (hasVariadicArgument ? "at least " : "")
                        << commandArgsCount << ", got " << args.size();
            return false;
        }
    }

    for (size_t i = 0; i < commandArgsCount; ++i)
    {
        auto commandType = command->arguments.get()[i].type;
        auto argType = args[i].type;

        if (commandType != Type::any and commandType != argType)
        {
            context.messageLine.error() << "Argument " << i << "; expected " << commandType << ", got " << argType;
            return false;
        }
    }

    return command->handler(args, flags, force, context);
}

static bool executeStatement(const TokensSpan& tokens, Context& context)
{
    switch (tokens[0].type)
    {
        case Token::Type::exclamation:
            return executeShellCommand(tokens.subspan(1), context);

        case Token::Type::identifier:
            executeCommand(tokens, context);
            break;

        case Token::Type::intLiteral:
            executeGoToCommand(tokens, context);
            break;

        default:
            context.messageLine.error() << "Unexpected statement beginning: " << tokens[0];
            return false;
    }

    return true;
}

bool executeCode(const std::string& line, Context& context)
{
    if (line.empty())
    {
        return true;
    }

    auto result = parse(line);

    if (not result)
    {
        context.messageLine.error() << result.error();
        return false;
    }

    const auto& tokens = result.value();

    auto current = tokens.begin();

    while (current != tokens.end())
    {
        auto lambda =
            [](const Token& token)
            {
                switch (token.type)
                {
                    case Token::Type::end:
                    case Token::Type::newline:
                    case Token::Type::semicolon:
                    case Token::Type::comment:
                        return true;
                    default:
                        return false;
                }
            };

        auto start = std::find_if_not(current, tokens.end(), lambda);
        auto end = std::find_if(start, tokens.end(), lambda);

        if (start == tokens.end())
        {
            break;
        }

        executeStatement(TokensSpan(start, end), context);

        current = end + 1;
    }

    return true;
}

}  // namespace core

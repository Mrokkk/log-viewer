#include "interpreter.hpp"

#include <algorithm>
#include <span>
#include <sstream>
#include <string>

#include "core/alias.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/lexer.hpp"
#include "core/type.hpp"
#include "core/variable.hpp"
#include "utils/string.hpp"

namespace core
{

using TokensSpan = std::span<const Token>;

static bool executeStatement(const TokensSpan& tokens, Context& context)
{
    if (tokens[0].type != Token::Type::identifier and
        tokens[0].type != Token::Type::intLiteral and
        tokens[0].type != Token::Type::exclamation)
    {
        *context.ui << error << "Unexpected token: " << tokens[0];
        return false;
    }

    std::string commandName;
    std::string tokenString(tokens[0].value.begin(), tokens[0].value.end());

    if (tokens[0].type == Token::Type::identifier)
    {
        auto& aliases = Aliases::map();

        if (aliases.contains(tokenString))
        {
            auto& alias = aliases[tokenString];
            commandName = alias.command;
        }
    }

    if (commandName.empty())
    {
        commandName = std::move(tokenString);
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
                *context.ui << error << "! in unexpected place";
                return false;
            }
            force = true;
            continue;
        }
        switch (tokens[i].type)
        {
            case Token::Type::flagLiteral:
                flags.insert(std::string(tokens[i].value));
                break;

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
                auto path = Variables::find("path");

                if (not path)
                {
                    *context.ui << error << "path variable does not exist";
                    return false;
                }

                auto value = path->reader(context);

                if (not value.string)
                {
                    *context.ui << error << "path not set";
                    return false;
                }

                args.emplace_back(
                    Argument{
                        .type = Type::string,
                        .string = *value.string,
                    });
                break;
            }

            case Token::Type::variableReference:
            {
                const auto variable = Variables::find(std::string(tokens[i].value));

                if (not variable)
                {
                    *context.ui << error << "No such variable: " << tokens[i].value;
                    return false;
                }

                const auto value = variable->reader(context);
                auto argument = Argument{
                    .type = variable->type,
                    .string{getVariableString(*variable, value)},
                };


                switch (variable->type)
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

            case Token::Type::comment:
                continue;

            default:
                *context.ui << error << "Unexpected token: " << tokens[i];
                return false;
        }
    }

    if (tokens[0].type == Token::Type::exclamation)
    {
        std::stringstream ss;
        for (const auto& arg : args)
        {
            ss << arg.string << ' ';
        }
        const auto string = ss.str();
        context.ui->executeShell(string);
        return true;
    }

    if (tokens[0].type == Token::Type::intLiteral)
    {
        const auto lineNumber = commandName | utils::to<long>;
        context.ui->scrollTo(lineNumber, context);
        return true;
    }

    const auto command = Commands::find(commandName);

    if (not command)
    {
        *context.ui << error << "Unknown command: " << commandName;
        return false;
    }

    const auto& commandArgs = command->arguments.get();

    const bool hasVariadicArgument = commandArgs.size() and commandArgs.back().type == Type::variadic;
    const auto commandArgsCount = hasVariadicArgument ? commandArgs.size() - 1 : commandArgs.size();

    if (commandArgsCount != args.size())
    {
        if (hasVariadicArgument and args.size() > commandArgsCount)
        {
        }
        else
        {
            *context.ui << error << "Invalid number of arguments passed to " << commandName
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
            *context.ui << error << "Argument " << i << "; expected " << commandType << ", got " << argType;
            return false;
        }
    }

    command->handler(args, flags, force, context);

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
        *context.ui << error << result.error();
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

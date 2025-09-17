#include "interpreter.hpp"

#include <algorithm>
#include <expected>
#include <span>
#include <string>
#include <string_view>

#include "core/alias.hpp"
#include "core/context.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/lexer.hpp"
#include "core/interpreter/symbol.hpp"
#include "core/interpreter/value.hpp"
#include "core/main_loop.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/type.hpp"
#include "utils/buffer.hpp"
#include "utils/maybe.hpp"
#include "utils/string.hpp"

namespace core::interpreter
{

using TokensSpan = std::span<const Token>;

static utils::Maybe<Value> resolveSymbol(const std::string& name, Context& context)
{
    const auto variable = Symbols::find(name);

    if (not variable) [[unlikely]]
    {
        context.messageLine.error() << "No such variable: " << name;
        return {};
    }

    return variable->value();
}

static utils::Maybe<Value> resolveCurrentPath(Context& context)
{
    if (not context.mainView.isCurrentWindowLoaded())
    {
        context.messageLine.error() << "No buffer loaded yet";
        return {};
    }

    auto buffer = context.mainView.currentBuffer();

    if (not buffer)
    {
        context.messageLine.error() << "No buffer loaded yet";
        return {};
    }

    return Value(Object::create(buffer->filePath()));
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
            const auto symbol = resolveSymbol(std::string(tokens[i].value), context);

            if (not symbol)
            {
                return false;
            }

            command << getValueString(*symbol);

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

    context.mainLoop->executeShell(command.str());

    return true;
}

static bool executeGoToCommand(const TokensSpan& tokens, Context& context)
{
    const auto lineNumber = tokens[0].value | utils::to<long>;
    context.mainView.scrollTo(static_cast<size_t>(lineNumber), context);
    return true;
}

static Type convert(const Value& value)
{
    switch (value.type())
    {
        case Value::Type::boolean:
            return Type::boolean;

        case Value::Type::integer:
            return Type::integer;

        case Value::Type::object:
            switch (value.object()->type())
            {
                case Object::Type::string:
                    return Type::string;
                default:
                    return Type::null;
            }

        case Value::Type::null:
        default:
            return Type::null;
    }
}

static utils::Maybe<int> getCommandFlagMask(const Command& command, const std::string_view& sv)
{
    for (const auto& [flag, flagMask] : command.flags.get())
    {
        if (flag == sv)
        {
            return flagMask;
        }
    }

    return {};
}

static bool executeCommand(const TokensSpan& tokens, Context& context)
{
    std::string commandName(tokens[0].value.begin(), tokens[0].value.end());

    const auto alias = Aliases::find(commandName);

    if (alias)
    {
        commandName = alias->command;
    }

    bool force{false};
    Values args;
    std::vector<std::string_view> flags;

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
                        args.emplace_back(Value(-1 * (tokens[i + 1].value | utils::to<long>)));
                        ++i;
                        break;
                    case Token::Type::identifier:
                        flags.emplace_back(tokens[i + 1].value);
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
                args.emplace_back(Value(tokens[i].value | utils::to<long>));
                break;

            case Token::Type::booleanLiteral:
                args.emplace_back(Value(tokens[i].value == "true"));
                break;

            case Token::Type::identifier:
            case Token::Type::stringLiteral:
                args.emplace_back(Value(Object::create(tokens[i].value)));
                break;

            case Token::Type::percent:
            {
                auto maybePath = resolveCurrentPath(context);

                if (not maybePath)
                {
                    return false;
                }

                args.emplace_back(*maybePath);
                break;
            }

            case Token::Type::dollar:
            {
                if (tokens[i + 1].type != Token::Type::identifier)
                {
                    goto error;
                }
                ++i;

                auto maybeValue = resolveSymbol(std::string(tokens[i].value), context);

                if (not maybeValue)
                {
                    return false;
                }

                args.emplace_back(std::move(*maybeValue));

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
        auto argType = args[i].type();

        if (commandType != Type::any and commandType != convert(args[i]))
        {
            context.messageLine.error() << "Argument " << i << "; expected " << commandType << ", got " << argType;
            return false;
        }
    }

    int flagsMask = 0;

    for (const auto& flag : flags)
    {
        auto result = getCommandFlagMask(*command, flag);

        if (not result) [[unlikely]]
        {
            context.messageLine.error() << "Unknown flag: " << flag;
            return false;
        }

        flagsMask |= *result;
    }

    return command->handler(args, flagsMask, force, context);
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

bool execute(const std::string& line, Context& context)
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

    const auto& tokens = *result;

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

}  // namespace core::interpreter

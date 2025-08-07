#include "operations.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <queue>
#include <span>
#include <sstream>
#include <string>
#include <thread>

#include <rapidfuzz/fuzz.hpp>
#include <re2/re2.h>

#include "core/alias.hpp"
#include "core/argparse.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/input_mapping.hpp"
#include "core/mapped_file.hpp"
#include "core/parser.hpp"
#include "core/type.hpp"
#include "core/variable.hpp"
#include "sys/system.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

namespace core
{

static CommandLineOption file(
    {
        .type = Type::string,
        .variant = Option::positional,
        .longName = "file",
        .help = "file to open",
    });

static CommandLineOption logFile(
    {
        .type = Type::string,
        .variant = Option::additional,
        .longName = "log-file",
        .shortName = 'l',
        .help = "file to which logs are saved",
    });

int run(int argc, char* const* argv, Context& context)
{
    sys::initialize();

    auto result = parseArgs(argc, argv);

    if (not result)
    {
        std::cerr << result.error() << '\n';
        return -1;
    }

    if (logFile)
    {
        logger.setLogFile(logFile->string);
    }

    initializeDefaultInputMapping(context);

    for (const auto& configFile : sys::getConfigFiles())
    {
        if (std::filesystem::exists(configFile))
        {
            logger << info << "sourcing " << configFile;
            executeCode("source \"" + configFile.string() + '\"', context);
            break;
        }
    }

    if (file)
    {
        asyncViewLoader(std::string(file->string), context);
    }

    context.ui->run(context);

    sys::finalize();
    logger.flushToStderr();

    return 0;
}

bool asyncViewLoader(std::string path, Context& context)
{
    if (not std::filesystem::exists(path))
    {
        *context.ui << error << path << ": no such file";
        return false;
    }

    auto view = context.ui->createView(path, context);

    auto& newFile = context.files.emplace_back(nullptr);

    std::thread(
        [path = std::move(path), &newFile, view, &context]
        {
            newFile = std::make_unique<MappedFile>(path);
            context.ui->attachFileToView(*newFile, view, context);
        }).detach();

    return true;
}

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

bool registerKeyPress(char c, Context& context)
{
    auto& inputMappings = InputMappings::map();
    auto& inputMappingSet = InputMappings::set();

    if (c == '\e')
    {
        context.inputState.state.clear();
        return true;
    }

    context.inputState.state += c;

    if (not inputMappingSet.contains(context.inputState.state))
    {
        context.inputState.state.clear();
        return false;
    }

    auto inputMappingIt = inputMappings.find(context.inputState.state);

    if (inputMappingIt != inputMappings.end())
    {
        executeCode(inputMappingIt->second.command, context);
        context.inputState.state.clear();
    }

    return true;
}

utils::Strings readCurrentDirectory()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    for (const auto& entry : directory_iterator(current))
    {
        if (entry.is_regular_file())
        {
            result.push_back(relative(entry.path(), current));
        }
    }
    return result;
}

utils::Strings readCurrentDirectoryRecursive()
{
    using namespace std::filesystem;
    utils::Strings result;
    const auto& current = current_path();
    for (const auto& entry : recursive_directory_iterator(current))
    {
        if (entry.is_regular_file())
        {
            result.push_back(relative(entry.path(), current));
        }
    }
    return result;
}

struct ScoredPath
{
    std::string* string;
    double       score;
};

bool operator<(const ScoredPath& lhs, const ScoredPath& rhs)
{
    return lhs.score < rhs.score;
}

std::priority_queue<ScoredPath> extract(
    const std::string& query,
    const utils::Strings& choices,
    const double score_cutoff = 0.0)
{
    std::priority_queue<ScoredPath> results;

    rapidfuzz::fuzz::CachedTokenRatio<std::string::value_type> scorer(query);

    for (const auto& choice : choices)
    {
        double score = scorer.similarity(choice, score_cutoff);

        if (score >= score_cutoff)
        {
            results.emplace(ScoredPath{.string = (std::string*)&choice, .score = score});
        }
    }

    return results;
}

utils::StringRefs fuzzyFilter(const utils::Strings& strings, const std::string& pattern)
{
    utils::StringRefs refs;

    if (pattern.empty())
    {
        for (const auto& string : strings)
        {
            refs.push_back(&string);
        }
        return refs;
    }

    auto values = extract(pattern, strings, 2);

    while (not values.empty())
    {
        refs.push_back(values.top().string);
        values.pop();
    }

    return refs;
}

static LineRefs grep(const std::string& pattern, const GrepOptions& options, const LineRefs& filter, MappedFile& file)
{
    LineRefs lines;
    size_t lineCount;
    std::function<size_t(size_t)> lineNumberTransform;

    if (not filter.empty())
    {
        lineNumberTransform = [&filter](size_t i){ return filter[i]; };
        lineCount = filter.size();
    }
    else
    {
        lineNumberTransform = [](size_t i){ return i; };
        lineCount = file.lineCount();
    }

    if (not options.regex)
    {
        if (options.inverted)
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (not file[lineIndex].contains(pattern))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (file[lineIndex].contains(pattern))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
    }
    else
    {
        RE2::Options reOptions;
        reOptions.set_log_errors(false);
        RE2 re(std::move(pattern), reOptions);

        if (options.inverted)
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (not RE2::PartialMatch(file[lineIndex], re))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < lineCount; ++i)
            {
                auto lineIndex = lineNumberTransform(i);
                if (RE2::PartialMatch(file[lineIndex], re))
                {
                    lines.emplace_back(lineIndex);
                }
            }
        }
    }

    return lines;
}

void asyncGrep(std::string pattern, GrepOptions options, const LineRefs& filter, MappedFile& file, std::function<void(LineRefs, float)> callback)
{
    std::thread(
        [pattern = std::move(pattern), &filter, &file, callback = std::move(callback), options]
        {
            LineRefs lines;
            auto time = utils::measureTime(
                [&lines, &pattern, &filter, &file, options]
                {
                    lines = grep(pattern, options, filter, file);
                });
            callback(std::move(lines), time);
        }).detach();
}

}  // namespace core

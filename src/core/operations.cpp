#include "operations.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <queue>
#include <span>
#include <sstream>
#include <string>
#include <thread>

#include <rapidfuzz/fuzz.hpp>

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
#include "utils/math.hpp"
#include "utils/string.hpp"

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
        context.currentView = asyncViewLoader(std::string(file->string), context);
    }

    context.ui->run(context);

    sys::finalize();
    logger.flushToStderr();

    return 0;
}

View* asyncViewLoader(std::string path, Context& context)
{
    if (not std::filesystem::exists(path))
    {
        *context.ui << error << path << ": no such file";
        return nullptr;
    }

    // FIXME: a workaround for the fact that vector can reallocate data,
    // while I keep references to old data in thread. I need to switch
    // to non-continguos data structures
    context.views.reserve(100);
    context.files.reserve(100);

    auto& view = context.views.emplace_back(
        View{
            .file = nullptr,
            .viewHeight = 0,
            .yoffset = 0,
            .xoffset = 0,
            .lineNrDigits = 0,
            .ringBuffer{0}
        });

    auto& newFile = context.files.emplace_back(nullptr);

    std::thread(
        [path = std::move(path), &newFile, &view, &context]
        {
            newFile = std::make_unique<MappedFile>(path);
            view.file = newFile.get();
            context.ui->execute([&newFile, &context, &view]
                {
                    *context.ui << info
                        << newFile->path() << ": lines: " << newFile->lineCount() << "; took "
                        << std::fixed << std::setprecision(3) << newFile->loadTime() << " s";
                    reloadView(view, context);
                });
        }).detach();

    return &view;
}

void reloadLines(View& view, Context& context)
{
    for (size_t i = view.yoffset; i < view.yoffset + view.viewHeight; ++i)
    {
        view.ringBuffer.pushBack(getLine(context, view, i));
    }
}

bool scrollLeft(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.xoffset == 0)
    {
        return true;
    }

    view.xoffset--;
    reloadLines(view, context);
    return true;
}

bool scrollRight(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    view.xoffset++;
    reloadLines(view, context);
    return true;
}

bool scrollLineDown(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.yoffset + view.viewHeight >= view.file->lineCount())
    {
        return true;
    }

    view.yoffset++;
    auto line = getLine(context, view, view.yoffset + view.viewHeight - 1);
    view.ringBuffer.pushBack(std::move(line));

    return true;
}

bool scrollLineUp(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset--;
    view.ringBuffer.pushFront(getLine(context, view, view.yoffset));

    return true;
}

bool scrollPageDown(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.yoffset + view.viewHeight >= view.file->lineCount())
    {
        return true;
    }

    view.yoffset = (view.yoffset + view.viewHeight)
        | utils::clamp(0ul, view.file->lineCount() - view.viewHeight);

    reloadLines(view, context);

    return true;
}

bool scrollPageUp(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset -= view.viewHeight;

    if ((long)view.yoffset < 0)
    {
        view.yoffset = 0;
    }

    reloadLines(view, context);

    return true;
}

bool scrollToBeginning(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset = 0;
    reloadLines(view, context);

    return true;
}

bool scrollToEnd(Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return true;
    }

    auto& view = *context.currentView;

    const auto lastViewLine = view.file->lineCount() - view.viewHeight;

    if (view.yoffset >= lastViewLine)
    {
        return true;
    }

    view.yoffset = lastViewLine;

    reloadLines(view, context);

    return true;
}

void scrollTo(size_t lineNumber, Context& context)
{
    if (not context.currentView or not context.currentView->file)
    {
        return;
    }

    auto& view = *context.currentView;

    lineNumber = lineNumber | utils::clamp(0ul, view.file->lineCount() - view.viewHeight);
    view.yoffset = lineNumber;
    reloadLines(view, context);
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
                if (not context.currentView or not context.currentView->file)
                {
                    *context.ui << error << "File path not set in current view";
                    return false;
                }
                const auto& path = context.currentView->file->path();
                args.emplace_back(
                    Argument{
                        .type = Type::string,
                        .string{path.begin(), path.end()},
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
        if (lineNumber == -1)
        {
            scrollToEnd(context);
        }
        else if (lineNumber < 0)
        {
            return false;
        }
        scrollTo(lineNumber, context);
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

    command->handler(args, force, context);

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

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        return context.showLineNumbers;
    }

    WRITER()
    {
        context.showLineNumbers = value.boolean;
        if (context.currentView and context.currentView->file)
        {
            reloadLines(*context.currentView, context);
        }
        return true;
    }
}

DEFINE_READONLY_VARIABLE(path, string, "Path to the opened file")
{
    READER()
    {
        if (not context.currentView or not context.currentView->file)
        {
            return nullptr;
        }
        return &context.currentView->file->path();
    }
}

}  // namespace core

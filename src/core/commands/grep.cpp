#include "core/buffer.hpp"
#include "core/event.hpp"
#include "core/events/buffer_loaded.hpp"
#include "core/grep_options.hpp"
#include "core/interpreter/command.hpp"
#include "core/interpreter/interpreter.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "utils/bitflag.hpp"
#include "utils/buffer.hpp"

namespace core
{

DEFINE_BITFLAG(GrepFlags, uint8_t,
{
    regex,
    caseInsensitive,
    inverted,
});

DEFINE_COMMAND(grep)
{
    HELP() = "grep current buffer";

    FLAGS()
    {
        return {
            {"c", GrepFlags::caseInsensitive},
            {"i", GrepFlags::inverted},
            {"r", GrepFlags::regex},
        };
    }

    ARGUMENTS()
    {
        return {
            {Type::string, "pattern"}
        };
    }

    EXECUTOR()
    {
        GrepFlags flags(flagsMask);

        auto parentWindow = context.mainView.currentWindowNode();

        if (not parentWindow) [[unlikely]]
        {
            context.messageLine.error() << "No buffer loaded yet";
            return false;
        }

        auto pattern = *args[0].string();

        GrepOptions options;
        std::string optionsString;

        if (flags[GrepFlags::regex])
        {
            options.regex = true;
            optionsString += 'r';
        }
        if (flags[GrepFlags::caseInsensitive])
        {
            options.caseInsensitive = true;
            optionsString += 'c';
        }
        if (flags[GrepFlags::inverted])
        {
            options.inverted = true;
            optionsString += 'i';
        }

        utils::Buffer buf;

        buf << pattern;

        if (not optionsString.empty())
        {
            buf << " [" << optionsString << ']';
        }

        auto& newWindow = context.mainView.createWindow(buf.str(), MainView::Parent::currentWindow, context);

        auto newBuffer = newWindow.buffer();

        newBuffer->grep(
            std::move(pattern),
            options,
            parentWindow->bufferId(),
            context,
            [&newWindow, &context, bufferName = buf.str()](TimeOrError result)
            {
                sendEvent<events::BufferLoaded>(InputSource::internal, context, std::move(result), newWindow);
            });

        return true;
    }
}

namespace commands
{

bool grep(const std::string& pattern, const GrepOptions& options, Context& context)
{
    std::string command = "grep ";

    if (options.regex)
    {
        command += "-r ";
    }
    if (options.caseInsensitive)
    {
        command += "-c ";
    }
    if (options.inverted)
    {
        command += "-i ";
    }

    command += '\"';
    command += pattern;
    command += '\"';

    return interpreter::execute(command, context);
}

}  // namespace commands

}  // namespace core

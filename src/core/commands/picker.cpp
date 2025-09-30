#include "core/context.hpp"
#include "core/interpreter/command.hpp"
#include "core/main_picker.hpp"

namespace core
{

DEFINE_COMMAND(files)
{
    HELP() = "show file picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::files);
        return true;
    }
}

DEFINE_COMMAND(bookmarks)
{
    HELP() = "show bookmarks picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::bookmarks);
        return true;
    }
}

DEFINE_COMMAND(commands)
{
    HELP() = "show commands picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::commands);
        return true;
    }
}

DEFINE_COMMAND(variables)
{
    HELP() = "show variables picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::variables);
        return true;
    }
}

DEFINE_COMMAND(messages)
{
    HELP() = "show messages picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::messages);
        return true;
    }
}

DEFINE_COMMAND(logs)
{
    HELP() = "show logs picker";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        switchMode(Mode::picker, context);
        context.mainPicker.enter(context, MainPicker::Type::logs);
        return true;
    }
}

}  // namespace core

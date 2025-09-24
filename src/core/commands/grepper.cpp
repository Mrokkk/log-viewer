#include "core/context.hpp"
#include "core/interpreter/command.hpp"
#include "core/main_view.hpp"
#include "core/mode.hpp"

namespace core
{

DEFINE_COMMAND(grepper)
{
    HELP() = "show grepper";

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
        if (not context.mainView.isCurrentWindowLoaded())
        {
            return false;
        }

        switchMode(Mode::grepper, context);

        return true;
    }
}

}  // namespace core

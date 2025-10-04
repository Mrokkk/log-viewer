#include "core/context.hpp"
#include "core/main_view.hpp"
#include "core/interpreter/command.hpp"

namespace core
{

DEFINE_COMMAND(bookmarks)
{
    HELP() = "toggle bookmarks pane";

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
        context.mainView.toggleBookmarksPane();
        return true;
    }
}

}  // namespace core

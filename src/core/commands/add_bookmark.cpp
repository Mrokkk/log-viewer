#include "core/interpreter/command.hpp"
#include "core/main_view.hpp"
#include "core/type.hpp"

namespace core
{

DEFINE_COMMAND(addBookmark)
{
    HELP() = "bookmark selected line";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            {Type::string, "name"},
        };
    };

    EXECUTOR()
    {
        auto string = *args[0].string();
        context.mainView.addBookmark(std::move(string), context);
        return true;
    }
}

}  // namespace core

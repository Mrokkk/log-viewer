#include "core/alias.hpp"
#include "core/context.hpp"
#include "core/interpreter/command.hpp"
#include "core/main_view.hpp"

namespace core
{

DEFINE_COMMAND(highlight)
{
    HELP() = "highlight selected pattern";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {
            {Type::string, "pattern"},
            {Type::string, "color"}
        };
    };

    EXECUTOR()
    {
        auto pattern = *args[0].string();
        auto colorString = *args[1].string();
        context.mainView.highlight(std::move(pattern), std::move(colorString), context);
        return true;
    }
}

DEFINE_ALIAS(hl, highlight);

}  // namespace core

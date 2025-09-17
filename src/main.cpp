#include "core/app.hpp"
#include "core/context.hpp"
#include "ui/ftxui_creator.hpp"

int main(int argc, char* const* argv)
{
    auto& context = core::Context::instance();
    ui::createFtxuiUserInterface(context);
    return core::run(argc, argv, context);
}

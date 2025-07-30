#include "core/context.hpp"
#include "core/operations.hpp"
#include "ui/ftxui.hpp"

int main(int argc, char* const* argv)
{
    auto context = core::Context::create();
    ui::createFtxuiUserInterface(context);

    return core::run(argc, argv, context);
}

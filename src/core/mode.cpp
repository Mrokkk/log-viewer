#include "mode.hpp"

#include <ostream>

#include "core/context.hpp"
#include "core/user_interface.hpp"

namespace core
{

bool switchMode(Mode newMode, core::Context& context)
{
    context.ui->onModeSwitch(newMode, context);
    context.mode = newMode;
    return true;
}

std::ostream& operator<<(std::ostream& os, Mode mode)
{
#define PRINT_MODE(MODE) \
    case Mode::MODE: \
        return os << #MODE
    switch (mode)
    {
        PRINT_MODE(normal);
        PRINT_MODE(visual);
        PRINT_MODE(command);
        PRINT_MODE(custom);
        default:
            return os << "unexpected{" << int(mode) << '}';
    }
}

}  // namespace core

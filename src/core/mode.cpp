#include "mode.hpp"

#include "core/context.hpp"
#include "core/user_interface.hpp"
#include "utils/buffer.hpp"

namespace core
{

bool switchMode(Mode newMode, core::Context& context)
{
    context.ui->onModeSwitch(newMode, context);
    context.mode = newMode;
    return true;
}

utils::Buffer& operator<<(utils::Buffer& buf, Mode mode)
{
#define PRINT_MODE(MODE) \
    case Mode::MODE: \
        return buf << #MODE
    switch (mode)
    {
        PRINT_MODE(normal);
        PRINT_MODE(visual);
        PRINT_MODE(command);
        PRINT_MODE(custom);
        default:
            return buf << "unexpected{" << int(mode) << '}';
    }
}

}  // namespace core

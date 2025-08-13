#include "mode.hpp"

#include "core/context.hpp"

namespace core
{

bool switchMode(Mode newMode, core::Context& context)
{
    context.mode = newMode;
    return true;
}

}  // namespace core

#pragma once

#include "core/fwd.hpp"

namespace core
{

enum class Mode
{
    normal,
    visual,
    command,
    custom,
};

bool switchMode(Mode newMode, core::Context& context);

}  // namespace core

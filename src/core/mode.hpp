#pragma once

#include <iosfwd>
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
std::ostream& operator<<(std::ostream& os, Mode mode);

}  // namespace core

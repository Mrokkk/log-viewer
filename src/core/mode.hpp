#pragma once

#include "core/fwd.hpp"
#include "utils/buffer.hpp"
#include "utils/fwd.hpp"

namespace core
{

enum class Mode
{
    normal,
    visual,
    command,
    grepper,
    picker,
    bookmarks,
};

bool switchMode(Mode newMode, Context& context);
utils::Buffer& operator<<(utils::Buffer& buf, Mode mode);

}  // namespace core

#pragma once

#include <string>

#include "core/fwd.hpp"

namespace core
{

struct InputState
{
    std::string state;
};

bool registerKeyPress(char c, core::Context& context);

}  // namespace core

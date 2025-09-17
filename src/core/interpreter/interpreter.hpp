#pragma once

#include <string>

#include "core/fwd.hpp"

namespace core::interpreter
{

bool execute(const std::string& line, Context& context);

}  // namespace core::interpreter

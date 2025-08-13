#pragma once

#include <string>

#include "core/fwd.hpp"

namespace core::commands
{

bool source(const std::string& filename, Context& context);

}  // namespace core::commands

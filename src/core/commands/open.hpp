#pragma once

#include <string>

#include "core/fwd.hpp"

namespace core::commands
{

bool open(const std::string& path, Context& context);

}  // namespace core::commands

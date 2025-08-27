#pragma once

#include <string>

#include "core/fwd.hpp"
#include "core/grep_options.hpp"

namespace core::commands
{

bool grep(const std::string& pattern, const GrepOptions& options, Context& context);

}  // namespace core::commands

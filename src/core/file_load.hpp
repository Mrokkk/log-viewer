#pragma once

#include <string>

#include "core/fwd.hpp"

namespace core
{

bool asyncLoadFile(std::string path, Context& context);

}  // namespace core

#pragma once

#include <string>

#include "utils/string.hpp"

namespace core
{

utils::StringRefs fuzzyFilter(const utils::Strings& strings, const std::string& pattern);

}  // namespace core

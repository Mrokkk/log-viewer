#pragma once

#include <expected>
#include <string>

#include "utils/string.hpp"

namespace core
{

using StringRefsOrError = std::expected<utils::StringRefs, std::string>;

StringRefsOrError fuzzyFilter(const utils::Strings& strings, const std::string& pattern);

}  // namespace core

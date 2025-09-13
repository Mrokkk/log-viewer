#pragma once

#include <flat_map>
#include <string>

#include "core/variable.hpp"

namespace core
{

using VariablesMap = std::flat_map<std::string, Variable>;

const VariablesMap& variablesMap();

}  // namespace core

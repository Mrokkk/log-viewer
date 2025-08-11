#pragma once

#include <string>
#include <flat_map>
#include <flat_set>
#include <string>

#include "core/fwd.hpp"

namespace core
{

struct InputState
{
    std::string state;
};

struct InputMapping
{
    std::string keys;
    std::string command;
};

using InputMappingMap = std::flat_map<std::string, InputMapping>;
using InputMappingSet = std::flat_set<std::string>;

struct InputMappings
{
    static InputMappingMap& map();
    static InputMappingSet& set();
};

bool addInputMapping(std::string keys, const std::string& command, bool force, Context& context);
void initializeDefaultInputMapping(core::Context& context);
bool registerKeyPress(char c, core::Context& context);

}  // namespace core

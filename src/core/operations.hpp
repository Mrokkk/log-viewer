#pragma once

#include <functional>
#include <string>

#include "core/fwd.hpp"
#include "core/mapped_file.hpp"
#include "utils/string.hpp"

namespace core
{

int run(int argc, char* const* argv, Context& context);
bool asyncViewLoader(std::string path, Context& context);
bool executeCode(const std::string& line, core::Context& context);
bool backspace(core::Context& context);
void initializeDefaultInputMapping(core::Context& context);
bool registerKeyPress(char c, core::Context& context);
utils::Strings readCurrentDirectory();
utils::Strings readCurrentDirectoryRecursive();
utils::StringRefs fuzzyFilter(const utils::Strings& strings, const std::string& pattern);
LineRefs grep(const std::string& pattern, const LineRefs& filter, MappedFile& file);
void asyncGrep(std::string pattern, const LineRefs& filter, MappedFile& file, std::function<void(LineRefs, float)> callback);

}  // namespace core

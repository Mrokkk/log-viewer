#pragma once

#include <cstddef>
#include <string>

#include "core/fwd.hpp"
#include "utils/string.hpp"

namespace core
{

int run(int argc, char* const* argv, Context& context);
View* asyncViewLoader(std::string path, Context& context);
std::string getLine(Context& context, View& view, size_t lineIndex);
void reloadLines(View& view, Context& context);
bool scrollLeft(core::Context& context);
bool scrollRight(core::Context& context);
bool scrollLineDown(core::Context& context);
bool scrollLineUp(core::Context& context);
bool scrollPageDown(core::Context& context);
bool scrollPageUp(core::Context& context);
bool scrollToBeginning(core::Context& context);
bool scrollToEnd(core::Context& context);
void scrollTo(size_t lineNumber, core::Context& context);
bool executeCode(const std::string& line, core::Context& context);
bool backspace(core::Context& context);
void reloadView(View& view, Context& context);
bool resize(core::Context& context);
void initializeDefaultInputMapping(core::Context& context);
bool registerKeyPress(char c, core::Context& context);
utils::Strings readCurrentDirectory();
utils::Strings readCurrentDirectoryRecursive();
utils::StringRefs fuzzyFilter(const utils::Strings& strings, const std::string& pattern);

}  // namespace core

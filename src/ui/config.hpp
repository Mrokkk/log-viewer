#pragma once

#include <cstdint>
#include <string>

namespace ui
{

struct Config
{
    bool        showLineNumbers = false;
    bool        absoluteLineNumbers = false;
    uint8_t     tabWidth = 4;
    std::string lineNumberSeparator = " ";
    std::string tabChar = "›";
};

}  // namespace ui

#pragma once

#include <cstdint>
#include <string>

namespace core
{

struct Config
{
    bool        showLineNumbers = false;
    bool        absoluteLineNumbers = false;
    uint8_t     scrollJump = 5;
    uint8_t     scrollOff = 3;
    uint8_t     fastMoveLen = 16;
    uint8_t     tabWidth = 4;
    std::string lineNumberSeparator = " ";
    std::string tabChar = "›";
};

}  // namespace core

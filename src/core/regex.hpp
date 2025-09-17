#pragma once

#include <string>

#include "utils/immobile.hpp"

namespace core
{

struct Regex : utils::Immobile
{
    Regex(std::string regex, bool caseInsensitive);
    ~Regex();

    bool ok();
    std::string error();
    bool partialMatch(const std::string_view& sv);

private:
    char mData[160];
};

}  // namespace regex

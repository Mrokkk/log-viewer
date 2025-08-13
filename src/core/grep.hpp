#pragma once

#include <functional>

#include "core/file.hpp"
#include "core/fwd.hpp"

namespace core
{

struct GrepOptions
{
    bool regex           = false;
    bool inverted        = false;
    bool caseInsensitive = false;
};

void asyncGrep(
    std::string pattern,
    GrepOptions options,
    const LineRefs& filter,
    File& file,
    std::function<void(LineRefs, float)> callback);

}  // namespace core

#pragma once

#include <functional>

#include "core/fwd.hpp"
#include "core/mapped_file.hpp"

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
    MappedFile& file,
    std::function<void(LineRefs, float)> callback);

}  // namespace core

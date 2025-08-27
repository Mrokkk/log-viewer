#pragma once

#include <cstddef>
#include <vector>

namespace core
{

struct Line final
{
    const size_t start;
    const size_t len;
};

using Lines = std::vector<Line>;
using LineRefs = std::vector<size_t>;

}  // namespace core

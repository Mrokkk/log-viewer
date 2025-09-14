#pragma once

#include <cstddef>
#include <vector>

namespace core
{

struct Line final
{
    size_t start;
    size_t len;
};

using Lines = std::vector<Line>;
using LineRefs = std::vector<size_t>;

}  // namespace core

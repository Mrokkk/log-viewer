#pragma once

#include <string>
#include <vector>

#include "core/mapped_file.hpp"
#include "utils/ring_buffer.hpp"

namespace core
{

struct View
{
    MappedFile*                    file;
    size_t                         viewHeight;
    size_t                         yoffset;
    size_t                         xoffset;
    size_t                         lineNrDigits;
    utils::RingBuffer<std::string> ringBuffer;
};

using Views = std::vector<View>;

}  // namespace core

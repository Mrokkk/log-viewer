#pragma once

#include <list>
#include <string>

#include "core/mapped_file.hpp"
#include "utils/ring_buffer.hpp"

namespace ui
{

struct View
{
    core::MappedFile*              file;
    size_t                         viewHeight;
    size_t                         yoffset;
    size_t                         xoffset;
    size_t                         lineNrDigits;
    utils::RingBuffer<std::string> ringBuffer;
};

using Views = std::list<View>;

}  // namespace ui

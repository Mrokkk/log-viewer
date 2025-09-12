#include "buffers.hpp"

#include "core/context.hpp"

namespace core
{

Buffer* getBuffer(BufferId id, Context& context)
{
    return context.buffers[id];
}

}  // namespace core

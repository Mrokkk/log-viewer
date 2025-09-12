#pragma once

#include "core/entity.hpp"
#include "core/entity_id.hpp"
#include "core/fwd.hpp"

namespace core
{

using BufferId = EntityId<Buffer>;
using Buffers = Entities<Buffer>;

Buffer* getBuffer(BufferId id, Context& context);

}  // namespace core

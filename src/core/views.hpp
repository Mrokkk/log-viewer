#pragma once

#include "core/entity.hpp"
#include "core/entity_id.hpp"
#include "core/fwd.hpp"

namespace core
{

using ViewId = EntityId<View>;
using Views = Entities<View>;

View* getView(ViewId id, Context& context);

}  // namespace core

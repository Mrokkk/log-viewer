#include "views.hpp"

#include "core/context.hpp"

namespace core
{

View* getView(ViewId id, Context& context)
{
    return context.views[id];
}

}  // namespace core

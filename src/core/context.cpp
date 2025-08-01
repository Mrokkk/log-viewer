#include "context.hpp"

namespace core
{

Context::Context() = default;
Context::~Context() = default;

Context Context::create()
{
    return core::Context();
}

}  // namespace core

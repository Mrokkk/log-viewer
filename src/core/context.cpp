#include "context.hpp"

namespace core
{

Context::Context()
    : mode(Mode::normal)
{
}

Context::~Context() = default;

Context Context::create()
{
    return core::Context();
}

}  // namespace core

#include "context.hpp"

namespace core
{

Context::Context()
    : currentView{nullptr}
    , showLineNumbers{false}
{
}

Context::~Context() = default;

Context Context::create()
{
    return core::Context();
}

}  // namespace core

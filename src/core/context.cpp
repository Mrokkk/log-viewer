#include "context.hpp"

namespace core
{

Context::Context()
    : currentView{nullptr}
    , terminalSize{0, 0}
    , showLineNumbers{false}
{
}

Context::~Context() = default;

Context Context::create()
{
    return core::Context();
}

}  // namespace core

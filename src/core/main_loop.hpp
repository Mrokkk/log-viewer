#pragma once

#include <functional>
#include <string>

#include "core/fwd.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct MainLoop : utils::Immobile
{
    using Closure = std::function<void()>;

    virtual ~MainLoop() = default;
    virtual void run(Context& context) = 0;
    virtual void quit(Context& context) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual void executeTask(Closure closure) = 0;
};

}  // namespace core

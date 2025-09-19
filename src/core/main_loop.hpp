#pragma once

#include <string>

#include "core/fwd.hpp"
#include "core/thread.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct MainLoop : utils::Immobile
{
    virtual ~MainLoop() = default;
    virtual void run(Context& context) = 0;
    virtual void quit(Context& context) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual void executeTask(Task task) = 0;
};

}  // namespace core

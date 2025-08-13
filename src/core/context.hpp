#pragma once

#include "core/file.hpp"
#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/mode.hpp"
#include "core/user_interface.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Context final : utils::Immobile
{
    ~Context();
    static Context create();

    Files            files;
    InputState       inputState;
    Mode             mode;
    UserInterfacePtr ui;

private:
    Context();
};

}  // namespace core

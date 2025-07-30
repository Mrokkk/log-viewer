#pragma once

#include "core/fwd.hpp"
#include "core/input_state.hpp"
#include "core/user_interface.hpp"
#include "core/view.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Context final : utils::Immobile
{
    ~Context();
    static Context create();

    Views            views;
    MappedFiles      files;
    View*            currentView;
    InputState       inputState;
    bool             showLineNumbers;
    UserInterfacePtr ui;

private:
    Context();
};

}  // namespace core

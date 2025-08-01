#pragma once

#include "core/fwd.hpp"
#include "core/input_state.hpp"
#include "core/mapped_file.hpp"
#include "core/user_interface.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Context final : utils::Immobile
{
    ~Context();
    static Context create();

    MappedFiles      files;
    InputState       inputState;
    UserInterfacePtr ui;

private:
    Context();
};

}  // namespace core

#pragma once

#include "core/fwd.hpp"
#include "core/mode.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Context final : utils::Immobile
{
    ~Context();
    static Context create();

private:
    Context();

    struct Data;
    Data* data_;

public:
    Mode           mode;
    Files&         files;
    InputState&    inputState;
    CommandLine&   commandLine;
    UserInterface* ui;
};

}  // namespace core

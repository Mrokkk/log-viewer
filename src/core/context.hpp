#pragma once

#include "core/entity.hpp"
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
    Data* mData;

public:
    bool           running;
    Mode           mode;
    Entities<View> views;
    InputState&    inputState;
    CommandLine&   commandLine;
    MessageLine&   messageLine;
    UserInterface* ui;
};

}  // namespace core

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

    constexpr static Context& instance()
    {
        return context;
    }

private:
    Context();

    struct Data;
    Data* mData;

    static Context context;

public:
    bool             running;
    Mode             mode;
    Entities<Buffer> buffers;
    InputState&      inputState;
    CommandLine&     commandLine;
    MessageLine&     messageLine;
    MainView&        mainView;
    Config&          config;
    UserInterface*   ui;
    MainLoop*        mainLoop;
};

}  // namespace core

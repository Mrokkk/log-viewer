#include "context.hpp"

#include "core/command_line.hpp"
#include "core/config.hpp"
#include "core/grepper.hpp"
#include "core/input.hpp"
#include "core/main_picker.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/user_interface.hpp"

namespace core
{

Context Context::context;

struct Context::Data final
{
    InputState  inputState;
    CommandLine commandLine;
    MessageLine messageLine;
    MainView    mainView;
    MainPicker  mainPicker;
    Grepper     grepper;
    Config      config;
};

Context::Context()
    : mData(new Data)
    , running(true)
    , mode(Mode::normal)
    , inputState(mData->inputState)
    , commandLine(mData->commandLine)
    , messageLine(mData->messageLine)
    , mainView(mData->mainView)
    , mainPicker(mData->mainPicker)
    , grepper(mData->grepper)
    , config(mData->config)
    , ui(nullptr)
    , mainLoop(nullptr)
{
}

Context::~Context()
{
    running = false;
    delete ui;
    delete mData;
}

}  // namespace core

#include "context.hpp"

#include "core/command_line.hpp"
#include "core/input.hpp"
#include "core/message_line.hpp"
#include "core/user_interface.hpp"

namespace core
{

struct Context::Data final
{
    InputState  inputState;
    CommandLine commandLine;
    MessageLine messageLine;
};

Context::Context()
    : mData(new Data)
    , running(true)
    , mode(Mode::normal)
    , inputState(mData->inputState)
    , commandLine(mData->commandLine)
    , messageLine(mData->messageLine)
{
}

Context::~Context()
{
    running = false;
    delete ui;
    delete mData;
}

Context Context::create()
{
    return core::Context();
}

}  // namespace core

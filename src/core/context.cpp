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
    : data_(new Data)
    , running(true)
    , mode(Mode::normal)
    , inputState(data_->inputState)
    , commandLine(data_->commandLine)
    , messageLine(data_->messageLine)
{
}

Context::~Context()
{
    running = false;
    delete ui;
    delete data_;
}

Context Context::create()
{
    return core::Context();
}

}  // namespace core

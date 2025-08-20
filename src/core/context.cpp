#include "context.hpp"

#include "core/command_line.hpp"
#include "core/files.hpp"
#include "core/input.hpp"
#include "core/user_interface.hpp"

namespace core
{

struct Context::Data final
{
    Files       files;
    InputState  inputState;
    CommandLine commandLine;
};

Context::Context()
    : data_(new Data)
    , mode(Mode::normal)
    , files(data_->files)
    , inputState(data_->inputState)
    , commandLine(data_->commandLine)
{
}

Context::~Context()
{
    delete data_;
    delete ui;
}

Context Context::create()
{
    return core::Context();
}

}  // namespace core

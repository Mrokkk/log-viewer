#pragma once

#include "core/fwd.hpp"
#include "core/grep_options.hpp"
#include "core/input.hpp"
#include "core/readline.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Grepper : utils::Immobile
{
    Grepper();
    ~Grepper();

    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);

    GrepOptions options;
    Readline    readline;

private:
    void accept(Context& context);
};

}  // namespace core

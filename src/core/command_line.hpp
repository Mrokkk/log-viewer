#pragma once

#include <string>

#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/readline.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct CommandLine : utils::Immobile
{
    CommandLine();
    ~CommandLine();

    Readline                            readline;
    const std::string&                  line;
    const size_t&                       cursor;
    const utils::StringViews&           completions;
    const utils::StringViews::iterator& currentCompletion;
    const std::string&                  suggestion;

private:
    void accept(Context& context);
};

void enterCommandLine(InputSource source, Context& context);
bool handleCommandLineKeyPress(KeyPress keyPress, InputSource source, Context& context);
void clearCommandLineHistory(Context& context);

}  // namespace core

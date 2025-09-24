#include "grepper.hpp"

#include "core/commands/grep.hpp"
#include "core/input.hpp"

namespace core
{

Grepper::Grepper()
{
    readline
        .enableSuggestions()
        .onAccept(
            [this](InputSource, Context& context)
            {
                accept(context);
            });
}

Grepper::~Grepper() = default;

bool Grepper::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    if (keyPress.type == KeyPress::Type::altCharacter)
    {
        switch (keyPress.value)
        {
            case 'r':
                options.regex ^= true;
                return false;
            case 'c':
                options.caseInsensitive ^= true;
                return false;
            case 'i':
                options.inverted ^= true;
                return false;
        }
    }

    return readline.handleKeyPress(keyPress, source, context);
}

void Grepper::accept(Context& context)
{
    if (readline.line().empty())
    {
        return;
    }

    commands::grep(readline.line(), options, context);

    readline.clear();
}

}  // namespace core

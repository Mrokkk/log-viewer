#pragma once

#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/picker.hpp"
#include "core/readline.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct CommandLine : utils::Immobile
{
    CommandLine();
    ~CommandLine();

    inline const Readline* operator->() const
    {
        return &readline;
    }

    void enter(InputSource source);
    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void clearHistory();

    Readline readline;

private:
    Picker mFilesPicker;
    Picker mHistoryPicker;

    void accept(Context& context);
};

}  // namespace core

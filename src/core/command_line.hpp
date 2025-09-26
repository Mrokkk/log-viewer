#pragma once

#include <cstdlib>

#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/picker.hpp"
#include "core/readline.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct CommandLine : utils::Immobile
{
    enum class Mode : char
    {
        command = ':',
        searchForward = '/',
        searchBackward = '?',
    };

    CommandLine();
    ~CommandLine();

    constexpr const Readline* operator->() const
    {
        switch (mMode)
        {
            case Mode::command:
                return &commandReadline;
            case Mode::searchForward:
            case Mode::searchBackward:
                return &searchReadline;
        }
        std::abort();
    }

    constexpr Readline& readline()
    {
        switch (mMode)
        {
            case Mode::command:
                return commandReadline;
            case Mode::searchForward:
            case Mode::searchBackward:
                return searchReadline;
        }
        std::abort();
    }

    constexpr Mode mode() const
    {
        return mMode;
    }

    void enter(InputSource source, Mode mode);
    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void clearHistory();
    void resize(int resx, int resy, Context& context);
    void initializeInputMapping(Context& context);

    Readline commandReadline;
    Readline searchReadline;

private:
    Mode   mMode;
    Picker mFilesPicker;
    Picker mHistoryPicker;

    void acceptCommand(Context& context);
    void acceptSearch(Context& context);
};

}  // namespace core

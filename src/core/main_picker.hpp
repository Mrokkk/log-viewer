#pragma once

#include "core/fwd.hpp"
#include "core/input.hpp"
#include "core/picker.hpp"
#include "core/readline.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct MainPicker : utils::Immobile
{
    enum Type : int
    {
        files,
        commands,
        variables,
        messages,
        logs,
        _last
    };

    MainPicker();
    ~MainPicker();

    void enter(Context& context, Type type = Type::files);
    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void resize(int resx, int resy, Context& context);

    constexpr const Readline& readline() const
    {
        return mReadline;
    }

    constexpr int& currentPickerIndex()
    {
        return mCurrentPicker;
    }

    constexpr Picker& currentPicker()
    {
        return mPickers[mCurrentPicker];
    }

private:
    void accept(Context& context);

    static utils::Strings feedFiles(Context& context);
    static utils::Strings feedCommands(Context& context);
    static utils::Strings feedVariables(Context& context);
    static utils::Strings feedMessages(Context& context);
    static utils::Strings feedLogs(Context& context);

    Readline mReadline;
    int      mCurrentPicker;
    Picker   mPickers[int(Type::_last)];
};

}  // namespace core

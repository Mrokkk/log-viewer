#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "core/input.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct Picker;

struct Readline final : utils::Immobile
{
    Readline();
    ~Readline();

    using OnAccept = std::function<void(InputSource, Context&)>;
    using RefreshCompletion = std::function<utils::StringViews(std::string_view)>;

    enum class AcceptBehaviour
    {
        replace,
        append,
    };

    void clear();
    void clearHistory();

    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void refreshCompletion();

    Readline& onAccept(OnAccept onAccept);
    Readline& setupCompletion(RefreshCompletion refreshCompletion);
    Readline& enableSuggestions();
    Readline& disableHistory();

    Readline& connectPicker(
        Picker& picker,
        char ctrlCharacter,
        AcceptBehaviour acceptBehaviour = AcceptBehaviour::replace);

    Readline& connectPicker(Picker& picker, Context& context);

    const std::string& line() const;
    const size_t& cursor() const;
    const std::string& suggestion() const;
    const utils::StringViews& completions() const;
    const utils::StringViewsIt& currentCompletion() const;
    const Picker* picker() const;
    const utils::Strings& history() const;

private:
    struct Impl;
    Impl* mPimpl;
};

}  // namespace core

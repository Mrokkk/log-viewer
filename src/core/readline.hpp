#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "core/input.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct Readline : utils::Immobile
{
    Readline();
    ~Readline();

    using OnAccept = std::function<void(InputSource, Context&)>;
    using RefreshCompletion = std::function<utils::StringViews(std::string_view)>;

    void clear();
    void clearHistory();

    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void refreshCompletion();

    Readline& onAccept(OnAccept onAccept);
    Readline& setupCompletion(RefreshCompletion refreshCompletion);
    Readline& enableSuggestions();
    Readline& disableHistory();

    const std::string& lineRef() const;
    const size_t& cursorRef() const;
    const std::string& suggestionRef() const;
    const utils::StringViews& completions() const;
    const utils::StringViews::iterator& currentCompletion() const;

private:
    struct Impl;
    Impl* mPimpl;
};

}  // namespace core

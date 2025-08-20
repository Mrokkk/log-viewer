#pragma once

#include <string>

#include "core/fwd.hpp"
#include "core/input.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct History final : utils::Immobile
{
    struct Iterator final
    {
        explicit Iterator(utils::Strings& content);
        ~Iterator();

        bool isBeginning() const;
        bool isEnd() const;
        operator bool() const;
        const std::string& operator*() const;
        Iterator& operator--();
        Iterator& operator++();
        void reset();

    private:
        utils::Strings&          content_;
        utils::Strings::iterator iterator_;
    };

    History();
    ~History();

    void pushBack(const std::string& entry);
    void clear();

    utils::Strings content;
    Iterator       current;
};

struct CommandLine : utils::Immobile
{
    CommandLine();
    ~CommandLine();

    std::string                  line;
    size_t                       cursor;
    History                      history;
    std::string                  savedLine;
    utils::StringViews           completions;
    utils::StringViews::iterator currentCompletion;
};

void enterCommandLine(InputSource source, core::Context& context);
bool handleCommandLineKeyPress(KeyPress keyPress, InputSource source, core::Context& context);

}  // namespace core

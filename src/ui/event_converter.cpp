#include "event_converter.hpp"

#include <ftxui/component/event.hpp>

#include "core/input.hpp"
#include "ui/event_hash.hpp"
#include "utils/hash_map.hpp"
#include "utils/maybe.hpp"

using namespace ftxui;

namespace ui
{

static utils::HashMap<Event, core::KeyPress> eventToKeyPress;

utils::Maybe<core::KeyPress> convertEvent(const ftxui::Event& event)
{
    auto node = eventToKeyPress.find(event);

    if (not node) [[unlikely]]
    {
        return {};
    }

    return node->second;
}

void initEventConverter()
{
    eventToKeyPress = {
        {Event::Return,         core::KeyPress::cr},
        {Event::Escape,         core::KeyPress::escape},
        {Event::ArrowDown,      core::KeyPress::arrowDown},
        {Event::ArrowUp,        core::KeyPress::arrowUp},
        {Event::ArrowLeft,      core::KeyPress::arrowLeft},
        {Event::ArrowRight,     core::KeyPress::arrowRight},
        {Event::ArrowDownCtrl,  core::KeyPress::ctrlArrowDown},
        {Event::ArrowUpCtrl,    core::KeyPress::ctrlArrowUp},
        {Event::ArrowLeftCtrl,  core::KeyPress::ctrlArrowLeft},
        {Event::ArrowRightCtrl, core::KeyPress::ctrlArrowRight},
        {Event::PageDown,       core::KeyPress::pageDown},
        {Event::PageUp,         core::KeyPress::pageUp},
        {Event::Backspace,      core::KeyPress::backspace},
        {Event::Delete,         core::KeyPress::del},
        {Event::Home,           core::KeyPress::home},
        {Event::End,            core::KeyPress::end},
        {Event::Tab,            core::KeyPress::tab},
        {Event::TabReverse,     core::KeyPress::shiftTab},
        {Event::F1,             core::KeyPress::function(1)},
        {Event::F2,             core::KeyPress::function(2)},
        {Event::F3,             core::KeyPress::function(3)},
        {Event::F4,             core::KeyPress::function(4)},
        {Event::F5,             core::KeyPress::function(5)},
        {Event::F6,             core::KeyPress::function(6)},
        {Event::F7,             core::KeyPress::function(7)},
        {Event::F8,             core::KeyPress::function(8)},
        {Event::F9,             core::KeyPress::function(9)},
        {Event::F10,            core::KeyPress::function(10)},
        {Event::F11,            core::KeyPress::function(11)},
        {Event::F12,            core::KeyPress::function(12)},
        {Event::Character('['), core::KeyPress::character('[')},
        {Event::Character('\\'), core::KeyPress::character('\\')},
        {Event::Character(']'), core::KeyPress::character(']')},
        {Event::Character('^'), core::KeyPress::character('^')},
        {Event::Character('_'), core::KeyPress::character('_')},
        {Event::Character('`'), core::KeyPress::character('`')},
        {Event::Character('{'), core::KeyPress::character('{')},
        {Event::Character('|'), core::KeyPress::character('|')},
        {Event::Character('}'), core::KeyPress::character('}')},
        {Event::Character('~'), core::KeyPress::character('~')},
        {Event::Character(' '), core::KeyPress::space},
        {Event::Special({'\e', '[', '1', '~'}), core::KeyPress::home},
        {Event::Special({'\e', '[', '4', '~'}), core::KeyPress::end},
        {Event::Special({'\e', '[', '1', ';', '2', 'A'}), core::KeyPress::shiftArrowUp},
        {Event::Special({'\e', '[', '1', ';', '2', 'B'}), core::KeyPress::shiftArrowDown},
        {Event::Special({'\e', '[', '1', ';', '2', 'D'}), core::KeyPress::shiftArrowLeft},
        {Event::Special({'\e', '[', '1', ';', '2', 'C'}), core::KeyPress::shiftArrowRight},
    };

    for (char c = '@'; c <= 'Z'; ++c)
    {
        char small = c + 0x20;
        char capital = c;
        char ctrl = c - 0x40;

        // Small letter
        eventToKeyPress.insert(
            Event::Character(std::string{small}),
            core::KeyPress::character(small));

        // Capital letter
        eventToKeyPress.insert(
            Event::Character(std::string{capital}),
            core::KeyPress::character(capital));

        auto ctrlCharacter = Event::Special(std::string{ctrl});

        if (not eventToKeyPress.find(ctrlCharacter))
        {
            // Ctrl+letter
            eventToKeyPress.insert(
                std::move(ctrlCharacter),
                core::KeyPress::ctrl(small));
        }

        // Alt+letter
        eventToKeyPress.insert(
            Event::Special(std::string{'\e', small}),
            core::KeyPress::alt(small));
    }

    for (char c = '!'; c <= '?'; ++c)
    {
        eventToKeyPress.insert(
            Event::Character(std::string{c}),
            core::KeyPress::character(c));
    }
}

}  // namespace ui

#pragma once

#include <string>
#include <vector>

#include "core/fwd.hpp"
#include "utils/bitflag.hpp"
#include "utils/fwd.hpp"

namespace std
{

template <typename Signature>
class function;

}  // namespace std

namespace core
{

struct KeyPress
{
    enum class Type : unsigned char
    {
        character,
        ctrlCharacter,
        altCharacter,
        escape,
        backspace,
        del,
        cr,
        space,
        arrowUp,
        arrowDown,
        arrowLeft,
        arrowRight,
        ctrlArrowUp,
        ctrlArrowDown,
        ctrlArrowLeft,
        ctrlArrowRight,
        shiftArrowUp,
        shiftArrowDown,
        shiftArrowLeft,
        shiftArrowRight,
        pageUp,
        pageDown,
        home,
        end,
        tab,
        shiftTab,
        function,
    };

    Type type;
    char value = 0;

    constexpr static inline KeyPress character(char c)
    {
        return {.type = Type::character, .value = c};
    }

    constexpr static inline KeyPress ctrl(char c)
    {
        return {.type = Type::ctrlCharacter, .value = c};
    }

    constexpr static inline KeyPress alt(char c)
    {
        return {.type = Type::altCharacter, .value = c};
    }

    constexpr static inline KeyPress function(char c)
    {
        return {.type = Type::function, .value = c};
    }

    std::string name() const;

    static KeyPress escape;
    static KeyPress cr;
    static KeyPress space;
    static KeyPress backspace;
    static KeyPress del;
    static KeyPress arrowUp;
    static KeyPress arrowDown;
    static KeyPress arrowLeft;
    static KeyPress arrowRight;
    static KeyPress ctrlArrowUp;
    static KeyPress ctrlArrowDown;
    static KeyPress ctrlArrowLeft;
    static KeyPress ctrlArrowRight;
    static KeyPress shiftArrowUp;
    static KeyPress shiftArrowDown;
    static KeyPress shiftArrowLeft;
    static KeyPress shiftArrowRight;
    static KeyPress pageUp;
    static KeyPress pageDown;
    static KeyPress home;
    static KeyPress end;
    static KeyPress tab;
    static KeyPress shiftTab;
};

using KeyPresses = std::vector<KeyPress>;

constexpr inline bool operator<(const KeyPress lhs, const KeyPress rhs)
{
    return lhs.type < rhs.type or (lhs.type == rhs.type and lhs.value < rhs.value);
}

constexpr inline bool operator==(const KeyPress lhs, const KeyPress rhs)
{
    return lhs.type == rhs.type and lhs.value == rhs.value;
}

struct KeyPressNode;

struct HelpEntry
{
    std::string name;
    std::string help;
};

using HelpEntries = std::vector<HelpEntry>;
using KeyPressNodes = std::vector<KeyPressNode*>;

struct InputState
{
    InputState();
    ~InputState();

    void clear();

    bool          assistedMode;
    KeyPresses    state;
    KeyPressNode* nodes;
    KeyPressNode* current;
    KeyPressNodes stack;
    HelpEntries   helpEntries;
};

DEFINE_BITFLAG(InputMappingFlags, char,
{
    normal,
    visual,
    command,
    bookmarks,
    force,
    noHelp,
});

enum class InputSource
{
    user,
    internal,
};

using BuiltinCommand = std::function<bool(InputSource, Context& context)>;

bool addInputMapping(
    std::string_view lhs,
    std::string_view rhs,
    InputMappingFlags flags,
    std::string help,
    Context& context);

bool addInputMapping(
    std::string_view lhs,
    BuiltinCommand rhs,
    InputMappingFlags flags,
    std::string help,
    Context& context);

utils::Buffer& operator<<(utils::Buffer& buf, const KeyPress k);
std::string inputStateString(Context& context);
void initializeInput(Context& context);

}  // namespace core

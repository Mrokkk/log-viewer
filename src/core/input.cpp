#define LOG_HEADER "Input"
#include "input.hpp"

#include <cctype>
#include <expected>
#include <flat_map>
#include <functional>
#include <string_view>

#include "core/command_line.hpp"
#include "core/context.hpp"
#include "core/logger.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "core/user_interface.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

namespace core
{

struct KeyPressNode
{
    using ChildrenMap = std::flat_map<KeyPress, KeyPressNode>;

    KeyPressNode(KeyPress keyPress);

    KeyPress    keyPress;
    int         refCount;
    ChildrenMap children;
};

struct InputMapping
{
    enum class Type
    {
        command,
        builtinCommand,
    };

    InputMapping(KeyPresses keySequence);
    InputMapping(BuiltinCommand command);
    ~InputMapping();

    InputMapping(const InputMapping& other);
    InputMapping& operator=(const InputMapping& other);

    InputMapping(InputMapping&& other);
    InputMapping& operator=(InputMapping&& other);

    bool operator()(core::Context& context);

    Type type;
    union
    {
        KeyPresses     keySequence;
        BuiltinCommand builtinCommand;
    };
};

using InputMappingMap = std::flat_map<KeyPresses, InputMapping>;

static InputMappingMap inputMappings;

KeyPress KeyPress::escape{         .type = KeyPress::Type::escape};
KeyPress KeyPress::cr{             .type = KeyPress::Type::cr};
KeyPress KeyPress::space{          .type = KeyPress::Type::space, .value = ' '};
KeyPress KeyPress::backspace{      .type = KeyPress::Type::backspace};
KeyPress KeyPress::del{            .type = KeyPress::Type::del};
KeyPress KeyPress::arrowUp{        .type = KeyPress::Type::arrowUp};
KeyPress KeyPress::arrowDown{      .type = KeyPress::Type::arrowDown};
KeyPress KeyPress::arrowLeft{      .type = KeyPress::Type::arrowLeft};
KeyPress KeyPress::arrowRight{     .type = KeyPress::Type::arrowRight};
KeyPress KeyPress::ctrlArrowUp{    .type = KeyPress::Type::ctrlArrowUp};
KeyPress KeyPress::ctrlArrowDown{  .type = KeyPress::Type::ctrlArrowDown};
KeyPress KeyPress::ctrlArrowLeft{  .type = KeyPress::Type::ctrlArrowLeft};
KeyPress KeyPress::ctrlArrowRight{ .type = KeyPress::Type::ctrlArrowRight};
KeyPress KeyPress::pageUp{         .type = Type::pageUp};
KeyPress KeyPress::pageDown{       .type = Type::pageDown};
KeyPress KeyPress::home{           .type = Type::home};
KeyPress KeyPress::end{            .type = Type::end};
KeyPress KeyPress::tab{            .type = Type::tab};
KeyPress KeyPress::shiftTab{       .type = Type::shiftTab};

utils::Buffer& operator<<(utils::Buffer& buf, const KeyPress k)
{
    switch (k.type)
    {
        case KeyPress::Type::character:
            return buf << "Character{" << k.value << '}';
        case KeyPress::Type::ctrlCharacter:
            return buf << "CtrlCharacter{" << k.value << '}';
        case KeyPress::Type::altCharacter:
            return buf << "AltCharacter{" << k.value << '}';
        case KeyPress::Type::escape:
            return buf << "Escape{}";
        case KeyPress::Type::backspace:
            return buf << "Backspace{}";
        case KeyPress::Type::del:
            return buf << "Delete{}";
        case KeyPress::Type::cr:
            return buf << "Cr{}";
        case KeyPress::Type::space:
            return buf << "Space{}";
        case KeyPress::Type::arrowUp:
            return buf << "ArrowUp{}";
        case KeyPress::Type::arrowDown:
            return buf << "ArrowDown{}";
        case KeyPress::Type::arrowLeft:
            return buf << "ArrowLeft{}";
        case KeyPress::Type::arrowRight:
            return buf << "ArrowRight{}";
        case KeyPress::Type::ctrlArrowUp:
            return buf << "CtrlArrowUp{}";
        case KeyPress::Type::ctrlArrowDown:
            return buf << "CtrlArrowDown{}";
        case KeyPress::Type::ctrlArrowLeft:
            return buf << "CtrlArrowLeft{}";
        case KeyPress::Type::ctrlArrowRight:
            return buf << "CtrlArrowRight{}";
        case KeyPress::Type::pageUp:
            return buf << "PageUp{}";
        case KeyPress::Type::pageDown:
            return buf << "PageDown{}";
        case KeyPress::Type::home:
            return buf << "Home{}";
        case KeyPress::Type::end:
            return buf << "End{}";
        case KeyPress::Type::tab:
            return buf << "Tab{}";
        case KeyPress::Type::shiftTab:
            return buf << "ShiftTab{}";
        case KeyPress::Type::function:
            return buf << "F" << int(k.value) << "{}";
    }

    return buf << "Unknown{" << int(k.type) << '}';
}

constexpr static std::string_view constevalName(KeyPress k)
{
    switch (k.type)
    {
        case KeyPress::Type::escape:         return "esc";
        case KeyPress::Type::backspace:      return "backspace";
        case KeyPress::Type::del:            return "del";
        case KeyPress::Type::cr:             return "cr";
        case KeyPress::Type::space:          return "space";
        case KeyPress::Type::arrowUp:        return "up";
        case KeyPress::Type::arrowDown:      return "down";
        case KeyPress::Type::arrowLeft:      return "left";
        case KeyPress::Type::arrowRight:     return "right";
        case KeyPress::Type::ctrlArrowUp:    return "c-up";
        case KeyPress::Type::ctrlArrowDown:  return "c-down";
        case KeyPress::Type::ctrlArrowLeft:  return "c-left";
        case KeyPress::Type::ctrlArrowRight: return "c-right";
        case KeyPress::Type::pageUp:         return "pgup";
        case KeyPress::Type::pageDown:       return "pgdown";
        case KeyPress::Type::home:           return "home";
        case KeyPress::Type::end:            return "end";
        case KeyPress::Type::tab:            return "tab";
        case KeyPress::Type::shiftTab:       return "s-tab";
        case KeyPress::Type::function:
            switch (k.value)
            {
                case 1:                      return "f1";
                case 2:                      return "f2";
                case 3:                      return "f3";
                case 4:                      return "f4";
                case 5:                      return "f5";
                case 6:                      return "f6";
                case 7:                      return "f7";
                case 8:                      return "f8";
                case 9:                      return "f9";
                case 10:                     return "f10";
                case 11:                     return "f11";
                case 12:                     return "f12";
            }
            break;
        case KeyPress::Type::character:
        case KeyPress::Type::ctrlCharacter:
        case KeyPress::Type::altCharacter:
            logger.error() << "unexpected key: " << k;
            std::abort();
    }
    return "";
}

std::string KeyPress::name() const
{
    utils::Buffer buf;
    switch (type)
    {
        case KeyPress::Type::character:
            buf << value;
            break;
        case KeyPress::Type::ctrlCharacter:
            buf << "<c-" << value << '>';
            break;
        case KeyPress::Type::altCharacter:
            buf << "<a-" << value << '>';
            break;
        case KeyPress::Type::function:
            buf << "<f" << int(value) << '>';
            break;
        default:
            buf << '<' << constevalName(*this) << '>';
            break;
    }
    return buf.str();
}

KeyPressNode::KeyPressNode(KeyPress k)
    : keyPress(k)
    , refCount(1)
{
}

InputState::InputState()
    : nodes(
        new KeyPressNode[3]{
            KeyPress::character('\0'),
            KeyPress::character('\0'),
            KeyPress::character('\0')
        })
    , current(nullptr)
{
    state.reserve(128);
}

InputState::~InputState()
{
    delete[] nodes;
}

InputMapping::InputMapping(KeyPresses k)
    : type(Type::command)
    , keySequence(k)
{
}

InputMapping::InputMapping(BuiltinCommand c)
    : type(Type::builtinCommand)
    , builtinCommand(c)
{
}

InputMapping::~InputMapping()
{
    switch (type)
    {
        case Type::command:
            std::destroy_at(&keySequence);
            break;
        case Type::builtinCommand:
            std::destroy_at(&builtinCommand);
            break;
    }
}

InputMapping::InputMapping(const InputMapping& other)
    : type(other.type)
{
    switch (type)
    {
        case Type::command:
            std::construct_at(&keySequence, other.keySequence);
            break;
        case Type::builtinCommand:
            std::construct_at(&builtinCommand, other.builtinCommand);
            break;
    }
}

InputMapping& InputMapping::operator=(const InputMapping& other)
{
    type = other.type;
    switch (type)
    {
        case Type::command:
            std::construct_at(&keySequence, other.keySequence);
            break;
        case Type::builtinCommand:
            std::construct_at(&builtinCommand, other.builtinCommand);
            break;
    }
    return *this;
}

InputMapping::InputMapping(InputMapping&& other)
    : type(other.type)
{
    switch (type)
    {
        case Type::command:
            std::construct_at(&keySequence, std::move(other.keySequence));
            break;
        case Type::builtinCommand:
            std::construct_at(&builtinCommand, std::move(other.builtinCommand));
            break;
    }
}

InputMapping& InputMapping::operator=(InputMapping&& other)
{
    type = other.type;
    switch (type)
    {
        case Type::command:
            std::construct_at(&keySequence, std::move(other.keySequence));
            break;
        case Type::builtinCommand:
            std::construct_at(&builtinCommand, std::move(other.builtinCommand));
            break;
    }
    return *this;
}

static std::expected<KeyPresses, std::string> convertKeys(std::string_view input)
{
#define KEYPRESS(K) \
    {constevalName(KeyPress::K), KeyPress::K}

    static std::flat_map<std::string_view, KeyPress> nameToKeyPress = {
        {"leader",    KeyPress::character(',')},
        KEYPRESS(escape),
        KEYPRESS(backspace),
        KEYPRESS(del),
        KEYPRESS(cr),
        KEYPRESS(space),
        KEYPRESS(arrowUp),
        KEYPRESS(arrowDown),
        KEYPRESS(arrowLeft),
        KEYPRESS(arrowRight),
        KEYPRESS(ctrlArrowUp),
        KEYPRESS(ctrlArrowDown),
        KEYPRESS(ctrlArrowLeft),
        KEYPRESS(ctrlArrowRight),
        KEYPRESS(pageUp),
        KEYPRESS(pageDown),
        KEYPRESS(home),
        KEYPRESS(tab),
        KEYPRESS(shiftTab),
        KEYPRESS(function(1)),
        KEYPRESS(function(2)),
        KEYPRESS(function(3)),
        KEYPRESS(function(4)),
        KEYPRESS(function(5)),
        KEYPRESS(function(6)),
        KEYPRESS(function(7)),
        KEYPRESS(function(8)),
        KEYPRESS(function(9)),
        KEYPRESS(function(10)),
        KEYPRESS(function(11)),
        KEYPRESS(function(12)),
    };

    KeyPresses keys;

    while (not input.empty())
    {
        if (input[0] == '<')
        {
            auto end = input.find('>');

            if (end == input.npos)
            {
                return std::unexpected("Missing closing > in key");
            }

            auto key = input.substr(1, end - 1);
            input.remove_prefix(end + 1);

            if (key[1] == '-')
            {
                switch (std::tolower(key[0]))
                {
                    case 'c':
                        keys.push_back(KeyPress::ctrl(std::tolower(key[2])));
                        continue;
                    case 'a':
                        keys.push_back(KeyPress::alt(std::tolower(key[2])));
                        continue;
                }
            }

            auto lowerKey = key | utils::lowerCase;

            if (auto it = nameToKeyPress.find(lowerKey); it != nameToKeyPress.end())
            {
                keys.push_back(it->second);
            }
            else
            {
                return std::unexpected("Unknown key: <" + lowerKey + '>');
            }
        }
        else
        {
            keys.push_back(KeyPress::character(input[0]));
            input.remove_prefix(1);
        }
    }

    return keys;
}

bool InputMapping::operator()(core::Context& context)
{
    switch (type)
    {
        case Type::command:
        {
            for (auto keyPress : keySequence)
            {
                registerKeyPress(keyPress, InputSource::internal, context);
            }
            return true;
        }

        case Type::builtinCommand:
        {
            return builtinCommand(context);
        }
    }
    return false;
}

static void updateKeyPressGraph(KeyPresses keySequence, KeyPressNode& root)
{
    auto currentNode = &root;

    for (size_t i = 0; i < keySequence.size(); ++i)
    {
        auto keyPress = keySequence[i];
        auto it = currentNode->children.find(keyPress);
        if (it == currentNode->children.end())
        {
            currentNode = &currentNode->children.emplace(
                std::make_pair(
                    keyPress,
                    KeyPressNode(keyPress)))
                .first->second;
        }
        else
        {
            currentNode = &it->second;
            ++currentNode->refCount;
        }
    }
}

static bool addInputMappingInternal(std::string_view lhs, InputMapping rhs, InputMappingFlags flags, Context& context)
{
    const auto converted = convertKeys(lhs);

    if (not converted) [[unlikely]]
    {
        context.messageLine.error() << converted.error();
        return false;
    }

    const auto& keySequence = converted.value();

    if (flags & InputMappingFlags::force)
    {
        inputMappings.erase(keySequence);
    }

    auto result = inputMappings.emplace(
        std::make_pair(
            keySequence,
            std::move(rhs)));

    if (not result.second) [[unlikely]]
    {
        context.messageLine.error() << "Mapping already exists (use ! to overwrite)";
        return false;
    }

    if (flags & InputMappingFlags::normal)
    {
        updateKeyPressGraph(keySequence, context.inputState.nodes[static_cast<int>(Mode::normal)]);
    }
    if (flags & InputMappingFlags::visual)
    {
        updateKeyPressGraph(keySequence, context.inputState.nodes[static_cast<int>(Mode::visual)]);
    }
    if (flags & InputMappingFlags::command)
    {
        updateKeyPressGraph(keySequence, context.inputState.nodes[static_cast<int>(Mode::command)]);
    }

    return true;
}

bool addInputMapping(std::string_view lhs, std::string_view rhs, InputMappingFlags flags, Context& context)
{
    const auto converted = convertKeys(rhs);

    if (not converted) [[unlikely]]
    {
        context.messageLine.error() << converted.error();
        return false;
    }

    return addInputMappingInternal(lhs, InputMapping(converted.value()), flags, context);
}

bool addInputMapping(std::string_view lhs, BuiltinCommand rhs, InputMappingFlags flags, Context& context)
{
    return addInputMappingInternal(lhs, InputMapping(rhs), flags, context);
}

static void clearInputState(InputState& inputState)
{
    inputState.state.clear();
    inputState.current = nullptr;
}

bool registerKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    auto& inputState = context.inputState;
    auto mode = context.mode;

    logger.debug() << keyPress << "; mode: " << mode;

    if (keyPress == KeyPress::character(':') and mode != Mode::command)
    {
        clearInputState(inputState);
        context.messageLine.clear();
        context.commandLine.enter(source);
        switchMode(Mode::command, context);
        return true;
    }
    else if (mode == Mode::command)
    {
        if (context.commandLine.handleKeyPress(keyPress, source, context))
        {
            if (context.mode == Mode::command)
            {
                switchMode(Mode::normal, context);
            }
        }
        return true;
    }
    else if (keyPress == KeyPress::escape)
    {
        clearInputState(inputState);
        switchMode(Mode::normal, context);
        return true;
    }

    static_assert(static_cast<int>(Mode::normal)  == 0);
    static_assert(static_cast<int>(Mode::visual)  == 1);
    static_assert(static_cast<int>(Mode::command) == 2);

    if (not inputState.current)
    {
        switch (mode)
        {
            case Mode::custom:
                // In this mode no key presses are handled here
                clearInputState(inputState);
                return false;
            default:
                inputState.current = &inputState.nodes[static_cast<int>(mode)];
        }
    }

    inputState.state.push_back(keyPress);

    const auto node = inputState.current->children.find(keyPress);

    if (node == inputState.current->children.end())
    {
        clearInputState(inputState);
        return false;
    }

    inputState.current = &node->second;

    if (const auto it = inputMappings.find(inputState.state); it != inputMappings.end())
    {
        clearInputState(inputState);
        it->second(context);
    }

    return true;
}

std::string inputStateString(Context& context)
{
    utils::Buffer buf;
    for (auto& k : context.inputState.state)
    {
        buf << k.name();
    }
    return buf.str();
}

void initializeInput(Context& context)
{
    addInputMapping(
        "gg",
        [](Context& context){ context.ui->scrollTo(Scroll::beginning, context); return true; },
        InputMappingFlags::normal | InputMappingFlags::visual,
        context);

    addInputMapping(
        "G",
        [](Context& context){ context.ui->scrollTo(Scroll::end, context); return true; },
        InputMappingFlags::normal | InputMappingFlags::visual,
        context);

    addInputMapping(
        "<c-c>",
        [](Context& context)
        {
            context.messageLine.info() << "Type :qa and press <Enter> to quit";
            return true;
        },
        InputMappingFlags::normal | InputMappingFlags::visual,
        context);

    addInputMapping(
        "<c-z>",
        [](Context&){ return true; },
        InputMappingFlags::normal | InputMappingFlags::visual,
        context);
}

}  // namespace core

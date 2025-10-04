#define LOG_HEADER "core::Input"
#include "input.hpp"

#include <cctype>
#include <expected>
#include <functional>
#include <string_view>
#include <vector>

#include "core/command_line.hpp"
#include "core/context.hpp"
#include "core/event.hpp"
#include "core/event_handler.hpp"
#include "core/events/key_press.hpp"
#include "core/grepper.hpp"
#include "core/logger.hpp"
#include "core/main_picker.hpp"
#include "core/main_view.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "utils/buffer.hpp"
#include "utils/hash_map.hpp"
#include "utils/maybe.hpp"
#include "utils/memory.hpp"
#include "utils/noncopyable.hpp"
#include "utils/string.hpp"

template<>
struct std::hash<core::KeyPress>
{
    std::size_t operator()(const core::KeyPress& k) const noexcept
    {
        return *reinterpret_cast<const uint16_t*>(&k);
    }
};

namespace core
{

namespace
{

struct InputMapping final : utils::NonCopyable
{
    enum class Type
    {
        command,
        builtinCommand,
    };

    constexpr InputMapping(KeyPresses k)
        : type(Type::command)
        , keySequence(k)
    {
    }

    constexpr InputMapping(BuiltinCommand c)
        : type(Type::builtinCommand)
        , builtinCommand(c)
    {
    }

    constexpr ~InputMapping()
    {
        switch (type)
        {
            case Type::command: utils::destroyAt(&keySequence); break;
            case Type::builtinCommand: utils::destroyAt(&builtinCommand); break;
        }
    }

    constexpr InputMapping(const InputMapping& other)
    {
        copyFrom(other);
    }

    constexpr InputMapping& operator=(const InputMapping& other)
    {
        copyFrom(other);
        return *this;
    }

    constexpr InputMapping(InputMapping&& other)
    {
        moveFrom(std::move(other));
    }

    constexpr InputMapping& operator=(InputMapping&& other)
    {
        moveFrom(std::move(other));
        return *this;
    }

private:
    constexpr void copyFrom(const InputMapping& other)
    {
        type = other.type;
        switch (type)
        {
            case Type::command:
                utils::constructAt(&keySequence, other.keySequence);
                break;

            case Type::builtinCommand:
                utils::constructAt(&builtinCommand, other.builtinCommand);
                break;
        }
    }

    constexpr void moveFrom(InputMapping&& other)
    {
        type = other.type;
        switch (type)
        {
            case Type::command:
                utils::constructAt(&keySequence, std::move(other.keySequence));
                break;

            case Type::builtinCommand:
                utils::constructAt(&builtinCommand, std::move(other.builtinCommand));
                break;
        }
    }

public:
    constexpr void operator()(InputSource source, Context& context) const
    {
        switch (type)
        {
            case Type::command:
                for (auto keyPress : keySequence)
                {
                    sendEvent<events::KeyPress>(InputSource::internal, context, keyPress);
                }
                break;

            case Type::builtinCommand:
                builtinCommand(source, context);
                break;
        }
    }

    Type type;
    union
    {
        KeyPresses     keySequence;
        BuiltinCommand builtinCommand;
    };
};

}  // namespace

struct KeyPressNode final : utils::NonCopyable
{
    constexpr static inline struct Root{} root = {};

    using ChildrenMap = utils::HashMap<KeyPress, KeyPressNode>;
    using MaybeInputMapping = utils::Maybe<InputMapping>;

    constexpr KeyPressNode(Root)
        : refCount(1)
    {
    }

    constexpr KeyPressNode(KeyPress k)
        : keyPress(k)
        , noHelp(false)
        , refCount(1)
    {
    }

    constexpr KeyPressNode(KeyPress k, InputMapping m, bool n, std::string h)
        : keyPress(k)
        , noHelp(n)
        , refCount(1)
        , mapping(m)
        , help(h)
    {
    }

    KeyPress          keyPress;
    bool              noHelp;
    int               refCount;
    ChildrenMap       children;
    MaybeInputMapping mapping;
    std::string       help;
};

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
KeyPress KeyPress::shiftArrowUp{   .type = KeyPress::Type::shiftArrowUp};
KeyPress KeyPress::shiftArrowDown{ .type = KeyPress::Type::shiftArrowDown};
KeyPress KeyPress::shiftArrowLeft{ .type = KeyPress::Type::shiftArrowLeft};
KeyPress KeyPress::shiftArrowRight{.type = KeyPress::Type::shiftArrowRight};
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
        case KeyPress::Type::character:       return buf << "Character{" << k.value << '}';
        case KeyPress::Type::ctrlCharacter:   return buf << "CtrlCharacter{" << k.value << '}';
        case KeyPress::Type::altCharacter:    return buf << "AltCharacter{" << k.value << '}';
        case KeyPress::Type::escape:          return buf << "Escape{}";
        case KeyPress::Type::backspace:       return buf << "Backspace{}";
        case KeyPress::Type::del:             return buf << "Delete{}";
        case KeyPress::Type::cr:              return buf << "Cr{}";
        case KeyPress::Type::space:           return buf << "Space{}";
        case KeyPress::Type::arrowUp:         return buf << "ArrowUp{}";
        case KeyPress::Type::arrowDown:       return buf << "ArrowDown{}";
        case KeyPress::Type::arrowLeft:       return buf << "ArrowLeft{}";
        case KeyPress::Type::arrowRight:      return buf << "ArrowRight{}";
        case KeyPress::Type::ctrlArrowUp:     return buf << "CtrlArrowUp{}";
        case KeyPress::Type::ctrlArrowDown:   return buf << "CtrlArrowDown{}";
        case KeyPress::Type::ctrlArrowLeft:   return buf << "CtrlArrowLeft{}";
        case KeyPress::Type::ctrlArrowRight:  return buf << "CtrlArrowRight{}";
        case KeyPress::Type::shiftArrowUp:    return buf << "ShiftArrowUp{}";
        case KeyPress::Type::shiftArrowDown:  return buf << "ShiftArrowDown{}";
        case KeyPress::Type::shiftArrowLeft:  return buf << "ShiftArrowLeft{}";
        case KeyPress::Type::shiftArrowRight: return buf << "ShiftArrowRight{}";
        case KeyPress::Type::pageUp:          return buf << "PageUp{}";
        case KeyPress::Type::pageDown:        return buf << "PageDown{}";
        case KeyPress::Type::home:            return buf << "Home{}";
        case KeyPress::Type::end:             return buf << "End{}";
        case KeyPress::Type::tab:             return buf << "Tab{}";
        case KeyPress::Type::shiftTab:        return buf << "ShiftTab{}";
        case KeyPress::Type::function:        return buf << "F" << int(k.value) << "{}";
    }

    return buf << "Unknown{" << int(k.type) << '}';
}

constexpr static std::string_view constevalName(KeyPress k)
{
    switch (k.type)
    {
        case KeyPress::Type::escape:          return "esc";
        case KeyPress::Type::backspace:       return "backspace";
        case KeyPress::Type::del:             return "del";
        case KeyPress::Type::cr:              return "cr";
        case KeyPress::Type::space:           return "space";
        case KeyPress::Type::arrowUp:         return "up";
        case KeyPress::Type::arrowDown:       return "down";
        case KeyPress::Type::arrowLeft:       return "left";
        case KeyPress::Type::arrowRight:      return "right";
        case KeyPress::Type::ctrlArrowUp:     return "c-up";
        case KeyPress::Type::ctrlArrowDown:   return "c-down";
        case KeyPress::Type::ctrlArrowLeft:   return "c-left";
        case KeyPress::Type::ctrlArrowRight:  return "c-right";
        case KeyPress::Type::shiftArrowUp:    return "s-up";
        case KeyPress::Type::shiftArrowDown:  return "s-down";
        case KeyPress::Type::shiftArrowLeft:  return "s-left";
        case KeyPress::Type::shiftArrowRight: return "s-right";
        case KeyPress::Type::pageUp:          return "pgup";
        case KeyPress::Type::pageDown:        return "pgdown";
        case KeyPress::Type::home:            return "home";
        case KeyPress::Type::end:             return "end";
        case KeyPress::Type::tab:             return "tab";
        case KeyPress::Type::shiftTab:        return "s-tab";
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

InputState::InputState()
    : nodes(new KeyPressNode[6]{
        KeyPressNode::root,
        KeyPressNode::root,
        KeyPressNode::root,
        KeyPressNode::root,
        KeyPressNode::root,
        KeyPressNode::root,
    })
    , current(nullptr)
{
    state.reserve(32);
}

InputState::~InputState()
{
    delete[] nodes;
}

void InputState::clear()
{
    assistedMode = false;
    state.clear();
    current = nullptr;
    stack.clear();
    helpEntries.clear();
}

static std::expected<KeyPresses, std::string> convertKeys(std::string_view input)
{
#define KEYPRESS(K) \
    {constevalName(KeyPress::K), KeyPress::K}

    static utils::HashMap<std::string_view, KeyPress> nameToKeyPress = {
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
        KEYPRESS(shiftArrowUp),
        KEYPRESS(shiftArrowDown),
        KEYPRESS(shiftArrowLeft),
        KEYPRESS(shiftArrowRight),
        KEYPRESS(pageUp),
        KEYPRESS(pageDown),
        KEYPRESS(home),
        KEYPRESS(end),
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

            if (key[1] == '-' and key.size() == 3)
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

            if (auto node = nameToKeyPress.find(lowerKey)) [[likely]]
            {
                keys.push_back(node->second);
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

namespace
{

using KeyPressNodeRefs = std::vector<KeyPressNode*>;

struct Transaction
{
    using Operations = std::vector<std::move_only_function<void()>>;
    KeyPressNodeRefs refCountIncreaseNodes;
    Operations       operations;
};

}  // namespace

static bool updateKeyPressTree(
    KeyPresses keySequence,
    KeyPressNode& root,
    InputMapping mapping,
    std::string help,
    bool noHelp,
    bool force,
    Transaction& transaction)
{
    auto currentNode = &root;

    for (size_t i = 0; i < keySequence.size() - 1; ++i)
    {
        auto keyPress = keySequence[i];
        auto it = currentNode->children.find(keyPress);
        if (not it)
        {
            transaction.operations.emplace_back(
                [currentNode, i, noHelp, keySequence = std::move(keySequence), mapping = std::move(mapping), help = std::move(help)] mutable
                {
                    for (; i < keySequence.size() - 1; ++i)
                    {
                        auto keyPress = keySequence[i];
                        currentNode = &currentNode->children.insert(
                            keyPress,
                            KeyPressNode(keyPress))
                            .second;
                    }

                    auto keyPress = keySequence.back();

                    currentNode->children.insert(
                        keyPress,
                        KeyPressNode(keyPress, std::move(mapping), noHelp, help));
                });
            return true;
        }
        else
        {
            currentNode = &it->second;
            transaction.refCountIncreaseNodes.emplace_back(currentNode);
        }
    }

    auto keyPress = keySequence.back();
    auto it = currentNode->children.find(keyPress);

    if (it)
    {
        if (force)
        {
            transaction.operations.emplace_back(
                [mapping = std::move(mapping), it]
                {
                    it->second.mapping = std::move(mapping);
                });
        }
        else
        {
            return false;
        }
    }
    else
    {
        transaction.operations.emplace_back(
            [mapping = std::move(mapping), help = std::move(help), currentNode, keyPress, noHelp]
            {
                currentNode->children.insert(
                    keyPress,
                    KeyPressNode(keyPress, std::move(mapping), noHelp, help));
            });
    }

    return true;
}

static bool addInputMappingInternal(
    std::string_view lhs,
    InputMapping rhs,
    InputMappingFlags flags,
    std::string help,
    Context& context)
{
    const auto converted = convertKeys(lhs);

    if (not converted) [[unlikely]]
    {
        context.messageLine.error() << converted.error();
        return false;
    }

    const auto& keySequence = *converted;

    const bool force = flags[InputMappingFlags::force];
    const bool noHelp = flags[InputMappingFlags::noHelp];

    KeyPressNodeRefs roots;
    roots.reserve(3);

    Transaction transaction;

    if (flags & InputMappingFlags::normal)
    {
        roots.emplace_back(&context.inputState.nodes[static_cast<int>(Mode::normal)]);
    }
    if (flags & InputMappingFlags::visual)
    {
        roots.emplace_back(&context.inputState.nodes[static_cast<int>(Mode::visual)]);
    }
    if (flags & InputMappingFlags::command)
    {
        roots.emplace_back(&context.inputState.nodes[static_cast<int>(Mode::command)]);
    }
    if (flags & InputMappingFlags::bookmarks)
    {
        roots.emplace_back(&context.inputState.nodes[static_cast<int>(Mode::bookmarks)]);
    }

    for (auto root : roots)
    {
        if (not updateKeyPressTree(
                keySequence,
                *root,
                rhs,
                help,
                noHelp,
                force,
                transaction)) [[unlikely]]
        {
            goto failure;
        }
    }

    for (auto& operation : transaction.operations)
    {
        operation();
    }

    for (auto node : transaction.refCountIncreaseNodes)
    {
        ++node->refCount;
    }

    return true;

failure:
    context.messageLine.error() << "Mapping already exists (use ! to overwrite)";
    return false;
}

bool addInputMapping(
    std::string_view lhs,
    std::string_view rhs,
    InputMappingFlags flags,
    std::string help,
    Context& context)
{
    const auto converted = convertKeys(rhs);

    if (not converted) [[unlikely]]
    {
        context.messageLine.error() << converted.error();
        return false;
    }

    return addInputMappingInternal(lhs, InputMapping(*converted), flags, std::move(help), context);
}

bool addInputMapping(
    std::string_view lhs,
    BuiltinCommand rhs,
    InputMappingFlags flags,
    std::string help,
    Context& context)
{
    return addInputMappingInternal(lhs, InputMapping(rhs), flags, std::move(help), context);
}

static void createHelpEntries(InputState& inputState, KeyPressNode& node)
{
    inputState.helpEntries.clear();
    node.children.forEach(
        [&inputState](const KeyPressNode::ChildrenMap::Node& node)
        {
            if (not node.second.noHelp)
            {
                inputState.helpEntries.emplace_back(
                    HelpEntry{
                       .name = node.first.name(),
                       .help = node.second.mapping
                            ? node.second.help
                            : "[More options]",
                    });
            }
        });
}

static void handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    auto& inputState = context.inputState;
    auto mode = context.mode;

    logger.debug() << keyPress << "; mode: " << mode;

    switch (mode)
    {
        case Mode::command:
            if (context.commandLine.handleKeyPress(keyPress, source, context))
            {
                if (context.mode == mode)
                {
                    switchMode(Mode::normal, context);
                }
            }
            return;

        case Mode::picker:
            if (context.mainPicker.handleKeyPress(keyPress, source, context))
            {
                if (context.mode == Mode::picker)
                {
                    switchMode(Mode::normal, context);
                }
            }
            return;

        case Mode::grepper:
            if (context.grepper.handleKeyPress(keyPress, source, context))
            {
                if (context.mode == Mode::grepper)
                {
                    switchMode(Mode::normal, context);
                }
            }
            return;

        case Mode::normal:
        case Mode::visual:
        case Mode::bookmarks:
        default:
            break;
    }

    if (keyPress == KeyPress::escape)
    {
        inputState.clear();
        context.mainView.escape();
        switchMode(Mode::normal, context);
        return;
    }
    else if (keyPress == KeyPress::backspace and inputState.current and inputState.assistedMode)
    {
        if (inputState.stack.empty())
        {
            return;
        }

        inputState.stack.pop_back();
        inputState.state.pop_back();

        inputState.current = inputState.stack.empty()
            ? &inputState.nodes[static_cast<int>(mode)]
            : inputState.stack.back();

        createHelpEntries(inputState, *inputState.current);

        return;
    }

    static_assert(static_cast<int>(Mode::normal)  == 0);
    static_assert(static_cast<int>(Mode::visual)  == 1);
    static_assert(static_cast<int>(Mode::command) == 2);

    if (not inputState.current)
    {
        inputState.current = &inputState.nodes[static_cast<int>(mode)];

        if (keyPress.type == KeyPress::Type::space)
        {
            inputState.assistedMode = true;
            createHelpEntries(inputState, inputState.nodes[static_cast<int>(mode)]);
            inputState.state.emplace_back(keyPress);
            return;
        }
    }

    inputState.state.push_back(keyPress);

    const auto node = inputState.current->children.find(keyPress);

    if (not node)
    {
        inputState.clear();
        return;
    }

    if (inputState.assistedMode)
    {
        inputState.stack.emplace_back(inputState.current);
    }

    inputState.current = &node->second;

    if (inputState.current->mapping)
    {
        auto& handler = *inputState.current->mapping;
        inputState.clear();
        handler(source, context);
    }
    else if (inputState.assistedMode)
    {
        createHelpEntries(inputState, *inputState.current);
    }

    return;
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

static void handleKeyPressEvent(EventPtr event, InputSource source, Context& context)
{
    handleKeyPress(event->cast<events::KeyPress>().keyPress, source, context);
}

void initializeInput(Context& context)
{
    registerEventHandler(Event::Type::KeyPress, &handleKeyPressEvent);

    addInputMapping(
        "<c-c>",
        [](InputSource, Context& context)
        {
            context.messageLine.info() << "Type :qa and press <Enter> to quit";
            return true;
        },
        InputMappingFlags::normal | InputMappingFlags::visual | InputMappingFlags::noHelp,
        "",
        context);

    addInputMapping(
        "<c-z>",
        [](InputSource, Context&){ return true; },
        InputMappingFlags::normal | InputMappingFlags::visual | InputMappingFlags::noHelp,
        "",
        context);

    context.commandLine.initializeInputMapping(context);
    context.mainView.initializeInputMapping(context);
}

}  // namespace core

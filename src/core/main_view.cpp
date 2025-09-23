#define LOG_HEADER "core::MainView"
#include "main_view.hpp"

#include <cstdint>
#include <string_view>

#include "core/assert.hpp"
#include "core/buffer.hpp"
#include "core/buffers.hpp"
#include "core/config.hpp"
#include "core/context.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/main_loop.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "core/palette.hpp"
#include "core/utf8.hpp"
#include "core/window.hpp"
#include "sys/system.hpp"
#include "utils/buffer.hpp"
#include "utils/format.hpp"
#include "utils/hash_map.hpp"
#include "utils/math.hpp"
#include "utils/maybe.hpp"
#include "utils/string.hpp"

namespace core
{

using utils::max;
using utils::min;
using utils::clamp;

struct MainView::Pattern
{
    enum class Type
    {
        matchPatternOnly,
        matchAfter,
        matchBefore,
    };

    Type      type;
    uint32_t  fgColor;
    uint32_t  bgColor;
};

struct MainView::Impl final : MainView
{
    Impl() = delete;

    constexpr static inline Impl& get(MainView* m)
    {
        return *static_cast<Impl*>(m);
    }

    enum class Movement
    {
        forward,
        backward,
    };

    constexpr const Glyphs& lineGlyphs(Window& w) const
    {
        return w.ringBuffer[w.ycurrent].glyphs;
    }

    constexpr size_t lineLength(Window& w) const
    {
        return w.ringBuffer[w.ycurrent].glyphs.size();
    }

    constexpr size_t linePosition(Window& w) const
    {
        return w.xcurrent + w.xoffset;
    }

    constexpr size_t lineIndex(Window& w) const
    {
        return w.ycurrent + w.yoffset;
    }

    constexpr size_t getAvailableViewHeight(WindowNode& node) const
    {
        return mHeight
            - 1 // command line
            - 1 // status line
            - (node.depth() + 1);
    }

    constexpr size_t getAvailableViewWidth(Window& w) const
    {
        return mWidth
            - (w.config->showLineNumbers * (w.lineNrDigits + w.config->lineNumberSeparator.get().length() + 1));
    }

    constexpr Buffer* currentLoadedBuffer()
    {
        if (not mCurrentWindowNode or not mCurrentWindowNode->loaded()) [[unlikely]]
        {
            return nullptr;
        }
        return mCurrentWindowNode->buffer();
    }

    constexpr WindowNode* currentLoadedWindowNode()
    {
        if (not mCurrentWindowNode or not mCurrentWindowNode->loaded()) [[unlikely]]
        {
            return nullptr;
        }
        return mCurrentWindowNode;
    }

    void removeWindow(WindowNode& node, Context&);
    void reloadWindow(WindowNode& node, Context& context);
    void reloadLines(Buffer& buffer, Window& w, Context& context);

    BufferLine getLine(Buffer& buffer, size_t lineIndex, Context& context);

    void alignCursor(Window& w);
    void updateSelection(Window& w);

    void applyHorizontalScrollJump(Window& w, Movement m);

    void left(Context& context);
    void right(Context& context);
    void up(Context& context);
    void down(Context& context);
    void pageUp(Context& context);
    void pageDown(Context& context);
    void goTo(Window& w, size_t lineNumber, Context& context);
    void goTo(size_t lineNumber, Context& context);
    void center(Context& context);
    void lineStart();
    void lineEnd();
    void scrollDown(Context& context);
    void scrollUp(Context& context);
    void scrollHorizontallyToCursor();

    void fastForward();
    void fastBackward();

    void wordBeginning();
    void wordEnd();

    void startSearch(std::string pattern, SearchDirection direction);
    void search(std::string pattern, SearchDirection direction, Context& context);
    void search(Context& context);

    void handleSearchResult(
        const SearchResult& result,
        const std::string& pattern,
        Window& w,
        Buffer& buffer,
        float time,
        Context& context);

    void selectionModeToggle(Context& context);
    void yank(Context& context);
    void yankSingle(Context& context);

    static utils::Maybe<Pattern> parse(std::string& pattern, std::string& colorString);

    WindowNode* getActiveLineView();
    void activeTablineLeft();
    void activeTablineRight();
    void activeTablineUp();
    void activeTablineDown();

    WindowNodes::iterator getWindowNodeIterator(WindowNode& node, WindowNodes& nodes);
};

MainView::MainView()
    : mRoot("root")
    , mCurrentWindowNode(nullptr)
{
    static_assert(sizeof(MainView::Impl) == sizeof(MainView));
}

MainView::~MainView() = default;

Buffer* MainView::currentBuffer() const
{
    if (not mCurrentWindowNode)
    {
        return nullptr;
    }

    return mCurrentWindowNode->buffer();
}

bool MainView::isCurrentWindowLoaded() const
{
    return mCurrentWindowNode and mCurrentWindowNode->loaded();
}

const char* MainView::activeFileName() const
{
    if (not mCurrentWindowNode)
    {
        return "[No Name]";
    }

    if (not mCurrentWindowNode->loaded())
    {
        return "[Loading]";
    }

    auto buffer = mCurrentWindowNode->buffer();

    if (not buffer) [[unlikely]]
    {
        return "[Closed]";
    }

    return buffer->filePath().c_str();
}

#define REGISTER_MAPPING(KEYS, FLAGS, ...) \
    addInputMapping( \
        KEYS, \
        [&]([[maybe_unused]] Context& context) { __VA_ARGS__; return true; }, \
        FLAGS, \
        context)

#define NORMAL InputMappingFlags::normal
#define VISUAL InputMappingFlags::visual

void MainView::initializeInputMapping(Context& context)
{
    auto& impl = Impl::get(this);

    REGISTER_MAPPING("gg",        NORMAL | VISUAL, impl.goTo(0, context));
    REGISTER_MAPPING("G",         NORMAL | VISUAL, impl.goTo(-1, context));
    REGISTER_MAPPING("h",         NORMAL | VISUAL, impl.left(context));
    REGISTER_MAPPING("l",         NORMAL | VISUAL, impl.right(context));
    REGISTER_MAPPING("k",         NORMAL | VISUAL, impl.up(context));
    REGISTER_MAPPING("j",         NORMAL | VISUAL, impl.down(context));
    REGISTER_MAPPING("H",         NORMAL | VISUAL, impl.fastBackward());
    REGISTER_MAPPING("L",         NORMAL | VISUAL, impl.fastForward());
    REGISTER_MAPPING("<left>",    NORMAL | VISUAL, impl.left(context));
    REGISTER_MAPPING("<right>",   NORMAL | VISUAL, impl.right(context));
    REGISTER_MAPPING("<up>",      NORMAL | VISUAL, impl.up(context));
    REGISTER_MAPPING("<down>",    NORMAL | VISUAL, impl.down(context));
    REGISTER_MAPPING("<pgup>",    NORMAL | VISUAL, impl.pageUp(context));
    REGISTER_MAPPING("<pgdown>",  NORMAL | VISUAL, impl.pageDown(context));
    REGISTER_MAPPING("<s-up>",    NORMAL | VISUAL, impl.pageUp(context));
    REGISTER_MAPPING("<s-down>",  NORMAL | VISUAL, impl.pageDown(context));
    REGISTER_MAPPING("b",         NORMAL | VISUAL, impl.wordBeginning());
    REGISTER_MAPPING("e",         NORMAL | VISUAL, impl.wordEnd());
    REGISTER_MAPPING("<c-e>",     NORMAL | VISUAL, impl.scrollDown(context));
    REGISTER_MAPPING("<c-y>",     NORMAL | VISUAL, impl.scrollUp(context));
    REGISTER_MAPPING("zz",        NORMAL | VISUAL, impl.center(context));
    REGISTER_MAPPING("zs",        NORMAL | VISUAL, impl.scrollHorizontallyToCursor());
    REGISTER_MAPPING("^",         NORMAL | VISUAL, impl.lineStart());
    REGISTER_MAPPING("<home>",    NORMAL | VISUAL, impl.lineStart());
    REGISTER_MAPPING("$",         NORMAL | VISUAL, impl.lineEnd());
    REGISTER_MAPPING("<end>",     NORMAL | VISUAL, impl.lineEnd());
    REGISTER_MAPPING("<c-left>",  NORMAL,          impl.activeTablineLeft());
    REGISTER_MAPPING("<c-right>", NORMAL,          impl.activeTablineRight());
    REGISTER_MAPPING("<c-up>",    NORMAL,          impl.activeTablineUp());
    REGISTER_MAPPING("<c-down>",  NORMAL,          impl.activeTablineDown());
    REGISTER_MAPPING("v",         NORMAL | VISUAL, impl.selectionModeToggle(context));
    REGISTER_MAPPING("y",         VISUAL,          impl.yank(context));
    REGISTER_MAPPING("yy",        NORMAL,          impl.yankSingle(context));
    REGISTER_MAPPING("n",         NORMAL,          mSearchMode = SearchDirection::forward; impl.search(context));
    REGISTER_MAPPING("N",         NORMAL,          mSearchMode = SearchDirection::backward; impl.search(context));
    REGISTER_MAPPING("<c-w>q",    NORMAL,          quitCurrentWindow(context));
}

void MainView::reloadAll(Context& context)
{
    mRoot.forEachRecursive(
        [this, &context](WindowNode& n)
        {
            if (n.type() == WindowNode::Type::window)
            {
                Impl::get(this).reloadWindow(n, context);
            }
        });
}

void MainView::resize(int width, int height, Context& context)
{
    mWidth = width;
    mHeight = height;
    reloadAll(context);
}

WindowNode& MainView::createWindow(std::string name, Parent parent, Context& context)
{
    auto [newBuffer, newBufferId] = context.buffers.allocate();

    auto parentView = parent == Parent::currentWindow
        ? mCurrentWindowNode->isBase()
            ? mCurrentWindowNode->parent()
            : mCurrentWindowNode
        : &mRoot;

    auto& group = (*parentView)
        .addChild(WindowNode::createGroup(std::move(name)))
        .setActive();

    if (parent == Parent::root)
    {
        group.depth(0);
        mActiveTabline = 0;
    }

    mCurrentWindowNode = &group
        .addChild(WindowNode::createWindow("base", newBufferId, context))
        .setActive();

    return *mCurrentWindowNode;
}

void MainView::bufferLoaded(TimeOrError result, WindowNode& node, Context& context)
{
    if (result) [[likely]]
    {
        auto newBuffer = node.buffer();

        if (not newBuffer)
        {
            return;
        }

        context.mainLoop->executeTask(
            [this, &node, &context]
            {
                node.loaded(true);
                Impl::get(this).reloadWindow(node, context);
            });

        context.messageLine.info()
            << node.parent()->name() << ": buffer loaded; lines: " << newBuffer->lineCount() << "; took "
            << (*result | utils::precision(3)) << " s";
    }
    else
    {
        const auto& error = result.error();
        if (error == BufferError::Aborted)
        {
            context.messageLine.info() << error;
        }
        else
        {
            context.messageLine.error() << error;

            context.mainLoop->executeTask(
                [result = std::move(result), &node, &context, this]
                {
                    Impl::get(this).removeWindow(*node.parent(), context);
                });
        }
    }
}

void MainView::escape()
{
    auto node = Impl::get(this).currentLoadedWindowNode();

    if (node)
    {
        node->window().selectionMode = false;
    }
}

void MainView::quitCurrentWindow(Context& context)
{
    if (not mCurrentWindowNode) [[unlikely]]
    {
        context.mainLoop->quit(context);
        return;
    }

    if (not mCurrentWindowNode->parent()) [[unlikely]]
    {
        return;
    }

    auto windowToClose = mCurrentWindowNode->isBase()
        ? mCurrentWindowNode->parent()
        : mCurrentWindowNode;

    Impl::get(this).removeWindow(*windowToClose, context);
}

void MainView::scrollTo(size_t lineNumber, Context& context)
{
    Impl::get(this).goTo(lineNumber, context);
}

void MainView::searchForward(std::string pattern, Context& context)
{
    auto& impl = Impl::get(this);
    impl.startSearch(std::move(pattern), SearchDirection::forward);
    impl.search(mSearchPattern, mSearchMode, context);
}

void MainView::searchBackward(std::string pattern, Context& context)
{
    auto& impl = Impl::get(this);
    impl.startSearch(std::move(pattern), SearchDirection::backward);
    impl.search(mSearchPattern, mSearchMode, context);
}

const static utils::HashMap<std::string, uint32_t> colors{
    {"black",   Palette::black},
    {"red",     Palette::red},
    {"green",   Palette::green},
    {"yellow",  Palette::yellow},
    {"blue",    Palette::blue},
    {"magenta", Palette::magenta},
    {"cyan",    Palette::cyan},
    {"white",   Palette::white},
    {"grey",    Palette::grey},
};

void MainView::highlight(std::string pattern, std::string colorString, Context& context)
{
    auto& impl = Impl::get(this);

    if (pattern.empty())
    {
        return;
    }

    auto data = impl.parse(pattern, colorString);

    if (not data)
    {
        return;
    }

    impl.mTrie.insert(
        std::move(pattern),
        std::move(*data));

    impl.reloadAll(context);
}

void MainView::Impl::removeWindow(WindowNode& node, Context&)
{
    assert(node.type() == WindowNode::Type::group, utils::format("WindowNode {}:{} type is {}", node.name(), &node, node.type()));
    assert(node.parent(), utils::format("View {}:{} has no parent", node.name(), &node));

    auto& children = node.parent()->children();
    auto nodeIt = Impl::get(this).getWindowNodeIterator(node, children);

    if (nodeIt == children.end())
    {
        return;
    }

    logger.debug() << "removing " << node.name();

    children.erase(nodeIt);

    auto nextIt = children.begin();

    if (nextIt != children.end())
    {
        auto& node = *nextIt;
        node->setActive();
        if (node->type() == WindowNode::Type::window)
        {
            mCurrentWindowNode = node.get();
        }
        else
        {
            mCurrentWindowNode = node->deepestActive();
        }
    }
    else
    {
        mCurrentWindowNode = nullptr;
    }

    if (mCurrentWindowNode and mCurrentWindowNode->depth() < mActiveTabline)
    {
        mActiveTabline = mCurrentWindowNode->depth();
    }
}

void MainView::Impl::reloadWindow(WindowNode& node, Context& context)
{
    if (not node.loaded()) [[unlikely]]
    {
        return;
    }

    auto buffer = node.buffer();

    if (not buffer) [[unlikely]]
    {
        return;
    }

    auto& w = node.window();

    w.lineCount = buffer->lineCount();
    w.lineNrDigits = utils::numberOfDigits(buffer->fileLineCount());
    w.width = getAvailableViewWidth(w);
    w.height = min(getAvailableViewHeight(node), w.lineCount);
    w.ringBuffer = RingBuffer(w.height);
    w.yoffset = clamp(w.yoffset, 0uz, buffer->lineCount() - w.height);
    w.ycurrent = clamp(w.ycurrent, 0uz, w.height - 1);

    reloadLines(*buffer, w, context);
}

constexpr static Glyph invalidGlyph(Utf8 c, uint32_t offset)
{
    constexpr static const uint8_t hex[] = "0123456789abcdef";
    return Glyph{
        .width = 4,
        .flags = GlyphFlags::invalid,
        .offset = offset,
        .characters = {
            '<',
            hex[c[0] >> 4],
            hex[c[0] & 0xf],
            '>'
        }
    };
}

constexpr static Glyph controlGlyph(Utf8 c, uint32_t offset)
{
    return Glyph{
        .width = 2,
        .flags = GlyphFlags::control,
        .offset = offset,
        .characters = {'^', c + 0x40}
    };
}

constexpr static Glyph tabGlyph(uint32_t offset, const Config& config)
{
    Glyph glyph{
        .width = config.tabWidth,
        .flags = GlyphFlags::control | GlyphFlags::whitespace,
        .offset = offset,
        .characters = {Utf8::parse(config.tabChar.get())},
    };
    if (config.tabWidth > 1)
    {
        for (uint8_t i = 1; i < config.tabWidth - 1; ++i)
        {
            glyph.characters[i] = ' ';
        }
    }
    return glyph;
}

constexpr static Glyph nativeGlyph(Utf8 c, uint32_t offset, GlyphFlags flags)
{
    return Glyph{
        .width = 1,
        .flags = flags,
        .offset = offset,
        .characters = {c}
    };
}

static Glyphs getGlyphs(std::string_view line, const Config& config)
{
    Glyphs glyphs;
    glyphs.reserve(line.size());

    const auto lineStart = line.data();

    while (not line.empty())
    {
        const uint32_t offset = line.data() - lineStart;
        const auto c = Utf8::parse(line);

        if (c.invalid) [[unlikely]]
        {
            glyphs.emplace_back(invalidGlyph(c, offset));
            line.remove_prefix(c.len);
            continue;
        }

        switch (c)
        {
            case '\0' ... '\b':
            case '\n' ... 0x1f:
                glyphs.emplace_back(controlGlyph(c, offset));
                break;

            case ' ':
                glyphs.emplace_back(nativeGlyph(c, offset, GlyphFlags::whitespace));
                break;

            case '\t':
                glyphs.emplace_back(tabGlyph(offset, config));
                break;

            default:
                glyphs.emplace_back(nativeGlyph(c, offset, {}));
                break;
        }

        line.remove_prefix(c.len);
    }
    return glyphs;
}

BufferLine MainView::Impl::getLine(Buffer& buffer, size_t lineIndex, Context& context)
{
    auto result = buffer.readLine(lineIndex);

    if (not result) [[unlikely]]
    {
        return {};
    }

    const auto& config = context.config;

    auto& data = *result;

    BufferLine line{
        .lineNumber = lineIndex,
        .absoluteLineNumber = buffer.absoluteLineNumber(lineIndex),
        .glyphs = getGlyphs(data, config),
    };

    line.segments.reserve(4);

    uint32_t fgColor = Palette::white;
    uint32_t prevFgColor = fgColor;

    auto startIt = line.glyphs.begin();

    auto ctx = mTrie.createScanContext();

    while (const auto node = mTrie.scan(data, ctx))
    {
        const auto nodeKeySize = node->first.size();
        const auto match = ctx.currentOffset - nodeKeySize;

        if (node->second.type == Pattern::Type::matchBefore)
        {
            fgColor = node->second.fgColor;
            for (auto& segment : line.segments)
            {
                if (segment.defColor)
                {
                    segment.color = fgColor;
                    segment.defColor = false;
                }
            }
        }

        auto endIt = line.glyphs.begin() + match;

        if (endIt != startIt)
        {
            line.segments.emplace_back(
                fgColor,
                true,
                GlyphsSpan(startIt, endIt));
        }

        startIt = endIt;
        endIt += nodeKeySize;

        if (node->second.type == Pattern::Type::matchAfter)
        {
            fgColor = node->second.fgColor;
        }
        else if (node->second.type == Pattern::Type::matchBefore)
        {
            fgColor = prevFgColor;
        }

        line.segments.emplace_back(
            node->second.fgColor,
            false,
            GlyphsSpan(startIt, endIt));

        startIt = endIt;
    }

    line.segments.emplace_back(
        fgColor,
        true,
        GlyphsSpan(startIt, line.glyphs.end()));

    return line;
}

void MainView::Impl::reloadLines(Buffer& buffer, Window& w, Context& context)
{
    w.ringBuffer.clear();
    for (size_t i = w.yoffset; i < w.yoffset + w.height; ++i)
    {
        w.ringBuffer.pushBack(getLine(buffer, i, context));
    }
}

void MainView::Impl::alignCursor(Window& w)
{
    const auto lineLen = lineLength(w);

    if (w.xoffset > lineLen)
    {
        if (lineLen > w.width)
        {
            w.xoffset = lineLen - w.width;
        }
        else
        {
            w.xoffset = 0;
        }
    }
    else
    {
        w.xcurrent = clamp(w.xcurrent, 0uz, lineLen - w.xoffset);
    }
}

void MainView::Impl::updateSelection(Window& w)
{
    if (not w.selectionMode)
    {
        return;
    }

    auto absoluteIndex = w.ycurrent + w.yoffset;

    if (absoluteIndex > w.selectionPivot)
    {
        w.selectionStart = w.selectionPivot;
        w.selectionEnd = absoluteIndex;
    }
    else if (absoluteIndex < w.selectionPivot)
    {
        w.selectionStart = absoluteIndex;
        w.selectionEnd = w.selectionPivot;
    }
    else
    {
        w.selectionEnd = w.selectionStart = absoluteIndex;
    }
}

void MainView::Impl::applyHorizontalScrollJump(Window& w, Movement m)
{
    if ((long)w.xcurrent < w.config->scrollOff and m == Movement::backward)
    { // Too close to the left margin, try to scroll
        size_t diff = w.config->scrollOff - (long)w.xcurrent;

        if (w.xoffset == 0)
        {
            return;
        }
        else
        {
            w.xcurrent = w.config->scrollOff - 1;
        }

        const auto jump = min(
            max(size_t(w.config->scrollJump), diff),
            w.xoffset);

        w.xcurrent += jump - diff + 1;
        w.xoffset -= jump;
    }
    else if (w.xcurrent >= w.width - w.config->scrollOff and m == Movement::forward)
    { // Too close to the right margin, try to scroll
        const auto lineLen = lineLength(w);
        const auto linePos = linePosition(w);

        if (linePos >= lineLen)
        {
            // If view already covers the position, don't scroll back
            if (w.xoffset >= lineLen - w.width + 1)
            {
                w.xcurrent = lineLen - w.xoffset;
            }
            else
            {
                w.xoffset = lineLen - w.width + 1;
                w.xcurrent = w.width - 1;
            }
            return;
        }

        // How far from scrollOff limit are we
        const auto diff = w.xcurrent - (w.width - w.config->scrollOff) + 1;

        if (w.xoffset + w.width >= lineLen)
        {
            return;
        }

        w.xcurrent = w.width - w.config->scrollOff;

        auto jump = min(
            max(size_t(w.config->scrollJump), diff),
            w.xoffset + w.width - lineLen);

        w.xoffset += jump;
        w.xcurrent -= jump - (diff - 1);
    }
}

#define GET_WINDOW_AND_BUFFER(WINDOW, BUFFER) \
    auto __n = currentLoadedWindowNode(); \
    if (not __n) [[unlikely]] { return; } \
    auto& WINDOW = __n->window(); \
    auto BUFFER = __n->buffer()

#define GET_WINDOW(WINDOW) \
    auto& WINDOW = *({ \
        auto node = currentLoadedWindowNode(); \
        if (not node) [[unlikely]] { return; } \
        &node->window(); \
    })

void MainView::Impl::left(Context& context)
{
    GET_WINDOW(w);

    --w.xcurrent;

    auto linePos = linePosition(w);

    if (long(linePos) < 0)
    {
        if (w.ycurrent > 0)
        {
            up(context);
            w.xcurrent = lineLength(w);
            w.xoffset = 0;
            applyHorizontalScrollJump(w, Movement::forward);
        }
        else
        {
            w.xcurrent = 0;
        }
        return;
    }

    applyHorizontalScrollJump(w, Movement::backward);
}

void MainView::Impl::right(Context& context)
{
    GET_WINDOW(w);

    const auto lineLen = lineLength(w);
    auto linePos = linePosition(w);
    auto lineId = lineIndex(w);

    // Already reached line end, move to the next line if possible
    if (linePos >= lineLen)
    {
        if (lineId < w.lineCount - 1)
        {
            down(context);
            w.xcurrent = 0;
            w.xoffset = 0;
        }
        return;
    }

    ++w.xcurrent;
    ++linePos;

    applyHorizontalScrollJump(w, Movement::forward);
}

void MainView::Impl::up(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.ycurrent > 0)
    {
        w.ycurrent--;
    }

    if (w.ycurrent < w.config->scrollOff)
    {
        if (w.yoffset == 0)
        {
            if (w.ycurrent == 0)
            {
                goto finish;
            }
        }
        else
        {
            w.ycurrent = w.config->scrollOff - 1;
        }

        int max = w.config->scrollJump;
        while (w.yoffset and max--)
        {
            w.yoffset--;
            w.ringBuffer.pushFront(getLine(*buffer, w.yoffset, context));
            w.ycurrent++;
        }
    }

    finish:
    {
        alignCursor(w);
        updateSelection(w);
    }
}

void MainView::Impl::down(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.ycurrent < w.height - 1)
    {
        ++w.ycurrent;
    }

    if (w.ycurrent >= w.height - w.config->scrollOff)
    {
        if (w.yoffset == w.lineCount - w.height)
        {
            if (w.ycurrent == w.height - 1)
            {
                goto finish;
            }
        }
        else
        {
            w.ycurrent = w.height - w.config->scrollOff;
        }

        int max = w.config->scrollJump;
        while (w.yoffset < w.lineCount - w.height and max--)
        {
            w.yoffset++;
            w.ringBuffer.pushBack(getLine(*buffer, w.yoffset + w.height - 1, context));
            w.ycurrent--;
        }
    }

    finish:
    {
        alignCursor(w);
        updateSelection(w);
    }
}

void MainView::Impl::pageUp(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.yoffset == 0)
    {
        w.ycurrent = 0;
        goto finish;
    }

    w.yoffset -= w.height;

    if (static_cast<long>(w.yoffset) < 0)
    {
        w.yoffset = 0;
    }

    reloadLines(*buffer, w, context);

    finish:
    {
        alignCursor(w);
        updateSelection(w);
    }
}

void MainView::Impl::pageDown(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.yoffset + w.height >= w.lineCount)
    {
        w.ycurrent = w.height - 1;
        goto finish;
    }

    w.yoffset = clamp(w.yoffset + w.height, 0uz, w.lineCount - w.height);

    reloadLines(*buffer, w, context);

    finish:
    {
        alignCursor(w);
        updateSelection(w);
    }
}

void MainView::Impl::goTo(Window& w, size_t lineNumber, Context& context)
{
    auto buffer = getBuffer(w.bufferId, context);
    w.yoffset =  clamp(lineNumber, 0uz, w.lineCount - w.height);
    w.ycurrent = clamp(lineNumber - w.yoffset, 0uz, w.height - 1);

    reloadLines(*buffer, w, context);
    alignCursor(w);
    updateSelection(w);
}

void MainView::Impl::goTo(size_t lineNumber, Context& context)
{
    GET_WINDOW(w);
    goTo(w, lineNumber, context);
}

void MainView::Impl::center(Context& context)
{
    GET_WINDOW(w);

    if (w.yoffset == w.lineCount - w.height and w.ycurrent > w.height / 2)
    {
        return;
    }

    if (w.yoffset == 0 and w.ycurrent < w.height / 2)
    {
        return;
    }

    goTo(w.ycurrent + w.yoffset - w.height / 2, context);
    w.ycurrent = w.height / 2;
}

void MainView::Impl::lineStart()
{
    GET_WINDOW(w);

    w.xoffset = 0;
    w.xcurrent = 0;

    updateSelection(w);
}

void MainView::Impl::lineEnd()
{
    GET_WINDOW(w);

    const auto lineLen = lineLength(w);
    const auto linePos = linePosition(w);

    if (linePos >= lineLen)
    {
        return;
    }

    w.xcurrent = lineLen;

    applyHorizontalScrollJump(w, Movement::forward);
    updateSelection(w);
}

void MainView::Impl::scrollDown(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.yoffset == w.lineCount - w.height)
    {
        return;
    }

    ++w.yoffset;
    w.ringBuffer.pushBack(getLine(*buffer, w.yoffset + w.height - 1, context));

    if (w.ycurrent > w.config->scrollOff)
    {
        --w.ycurrent;
    }
}

void MainView::Impl::scrollUp(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (w.yoffset > 0)
    {
        --w.yoffset;
        w.ringBuffer.pushFront(getLine(*buffer, w.yoffset, context));

        if (w.ycurrent < w.height - w.config->scrollOff - 1)
        {
            ++w.ycurrent;
        }
    }
}

void MainView::Impl::scrollHorizontallyToCursor()
{
    GET_WINDOW(w);

    w.xoffset += w.xcurrent;
    w.xcurrent = 0;
}

void MainView::Impl::fastForward()
{
    GET_WINDOW(w);

    w.xcurrent += min(lineLength(w) - linePosition(w), size_t(w.config->fastMoveLen));

    applyHorizontalScrollJump(w, Movement::forward);
}

void MainView::Impl::fastBackward()
{
    GET_WINDOW(w);

    w.xcurrent -= min(linePosition(w), size_t(w.config->fastMoveLen));

    applyHorizontalScrollJump(w, Movement::backward);
}

constexpr auto stopFlags = GlyphFlags::whitespace;

void MainView::Impl::wordBeginning()
{
    GET_WINDOW(w);

    const auto& glyphs = lineGlyphs(w);
    const auto origLinePos = linePosition(w);
    auto linePos = origLinePos;

    if (linePos < 1)
    {
        w.xcurrent = 0;
        w.xoffset = 0;
        return;
    }

    while (long(--linePos) >= 0 and (glyphs[linePos].flags & stopFlags));

    for (long i = linePos; i >= 0; --i)
    {
        if (glyphs[i].flags & stopFlags)
        {
            w.xcurrent -= origLinePos - i - 1;
            applyHorizontalScrollJump(w, Movement::backward);
            return;
        }
    }

    w.xcurrent = 0;
    w.xoffset = 0;
}

void MainView::Impl::wordEnd()
{
    GET_WINDOW(w);

    const auto& glyphs = lineGlyphs(w);
    const auto origLinePos = linePosition(w);
    const auto lineLen = lineLength(w);
    auto linePos = origLinePos;

    while (++linePos < lineLen and (glyphs[linePos].flags & stopFlags));

    for (auto i = linePos; i < lineLen; ++i)
    {
        if (glyphs[i].flags & stopFlags)
        {
            w.xcurrent += i - origLinePos - 1;
            applyHorizontalScrollJump(w, Movement::forward);
            return;
        }
    }

    lineEnd();
}

void MainView::Impl::startSearch(std::string pattern, SearchDirection direction)
{
    GET_WINDOW(w);

    mTrie.erase(mSearchPattern);

    mSearchPattern = std::move(pattern);
    mSearchMode = direction;

    mTrie.insert(mSearchPattern, Pattern{.type = Pattern::Type::matchPatternOnly, .fgColor = Palette::magenta});

    w.foundAnything = false;
}

void MainView::Impl::search(std::string pattern, SearchDirection direction, Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    if (mSearchPattern.empty()) [[unlikely]]
    {
        return;
    }

    w.pendingSearch = true;

    buffer->search(
        SearchRequest{
            .direction = direction,
            .continuation = w.foundAnything,
            .startLineIndex = lineIndex(w),
            .startLinePosition = linePosition(w),
            .pattern = pattern,
        },
        [this, &context, &w, buffer, pattern](SearchResult result, float time)
        {
            if (not context.running)
            {
                return;
            }

            context.mainLoop->executeTask(
                [this, &context, &w, buffer, pattern = std::move(pattern), result, time]
                {
                    handleSearchResult(result, pattern, w, *buffer, time, context);
                });
        });
}

void MainView::Impl::search(Context& context)
{
    GET_WINDOW(w);

    if (w.pendingSearch)
    {
        return;
    }

    search(mSearchPattern, mSearchMode, context);
}

void MainView::Impl::handleSearchResult(
    const SearchResult& result,
    const std::string& pattern,
    Window& w,
    Buffer& buffer,
    float time,
    Context& context)
{
    if (result.aborted)
    {
        context.messageLine.error() << "Aborted search: " << pattern;
        return;
    }

    w.pendingSearch = false;

    if (not result.valid)
    {
        if (w.foundAnything)
        {
            context.messageLine.error() << "Search hit "
                << (mSearchMode == SearchDirection::forward ? "bottom" : "top")
                << ": " << pattern;
        }
        else
        {
            context.messageLine.error() << "Pattern not found: " << pattern;
        }
        return;
    }

    w.foundAnything = true;

    if (result.lineIndex >= w.yoffset + w.height)
    {
        goTo(w, result.lineIndex, context);
    }
    else if (result.lineIndex < w.yoffset)
    {
        goTo(w, result.lineIndex, context);
    }

    w.ycurrent = result.lineIndex - w.yoffset;

    const auto& glyphs = lineGlyphs(w);

    bool found{false};
    size_t currentPos = 0;

    for (const auto& glyph : glyphs)
    {
        if (glyph.offset >= result.linePosition)
        {
            found = true;
            break;
        }
        ++currentPos;
    }

    if (not found) [[unlikely]]
    {
        logger.error() << "cannot find position " << result.linePosition << " in line " << result.lineIndex;
        return;
    }

    if (currentPos >= w.width)
    {
        w.xoffset = currentPos;
        w.xcurrent = 0;
        applyHorizontalScrollJump(w, Movement::backward);
    }
    else
    {
        w.xoffset = 0;
        w.xcurrent = currentPos;
        applyHorizontalScrollJump(w, Movement::forward);
    }

    alignCursor(w);

    if (w.config->highlightSearch)
    {
        reloadLines(buffer, w, context);
    }

    if (time > 0.01)
    {
        context.messageLine.info() << "took " << (time | utils::precision(3)) << " s";
    }
    else
    {
        context.messageLine.clear();
    }
}

void MainView::Impl::selectionModeToggle(Context& context)
{
    GET_WINDOW(w);

    if ((w.selectionMode ^= true))
    {
        switchMode(core::Mode::visual, context);
        context.mode = core::Mode::visual;
        w.selectionPivot
            = w.selectionEnd
            = w.selectionStart
            = w.ycurrent + w.yoffset;
    }
    else
    {
        switchMode(core::Mode::normal, context);
    }
}

void MainView::Impl::yank(Context& context)
{
    constexpr size_t MAX_LINES_COPIED = 2000;

    GET_WINDOW_AND_BUFFER(w, buffer);

    if (not w.selectionMode)
    {
        return;
    }

    auto lineCount = w.selectionEnd - w.selectionStart + 1;

    if (lineCount > MAX_LINES_COPIED)
    {
        context.messageLine.error() << "cannot yank more than " << MAX_LINES_COPIED << " lines";
        return;
    }

    utils::Buffer buf;

    for (auto i = w.selectionStart; i <= w.selectionEnd; ++i)
    {
        if (const auto result = buffer->readLine(i))
        {
            buf << *result << '\n';
        }
    }

    sys::copyToClipboard(buf.str());

    w.selectionMode = false;
    switchMode(core::Mode::normal, context);

    context.messageLine.info() << lineCount << " lines copied to clipboard";
}

void MainView::Impl::yankSingle(Context& context)
{
    GET_WINDOW_AND_BUFFER(w, buffer);

    utils::Buffer buf;

    if (const auto result = buffer->readLine(w.ycurrent + w.yoffset))
    {
        buf << *result << '\n';
    }

    sys::copyToClipboard(buf.str());

    w.selectionMode = false;

    context.messageLine.info() << "1 line copied to clipboard";
}

utils::Maybe<MainView::Pattern> MainView::Impl::parse(std::string& pattern, std::string& colorString)
{
    uint32_t color;
    colorString = colorString | utils::lowerCase;

    if (const auto node = colors.find(colorString))
    {
        color = node->second;
    }
    else
    {
        auto converted = strtoul(colorString.c_str(), nullptr, 16);
        if (converted == ULONG_MAX)
        {
            return {};
        }
        color = converted;
    }

    Pattern::Type type;

    if (pattern[0] == '*')
    {
        type = Pattern::Type::matchBefore;
        pattern = pattern.substr(1);
    }
    else if (pattern.back() == '*')
    {
        pattern.pop_back();
        type = Pattern::Type::matchAfter;
    }
    else
    {
        type = Pattern::Type::matchPatternOnly;
    }

    return Pattern{
        .type = type,
        .fgColor = color,
    };
}

WindowNode* MainView::Impl::getActiveLineView()
{
    auto node = mRoot.activeChild();

    while (node->activeChild())
    {
        if (node->depth() == mActiveTabline)
        {
            break;
        }
        node = node->activeChild();
    }

    return node;
}

void MainView::Impl::activeTablineLeft()
{
    auto prev = getActiveLineView()->prev();

    if (prev)
    {
        prev->setActive();
        mCurrentWindowNode = prev->parent()->deepestActive();
    }
}

void MainView::Impl::activeTablineRight()
{
    auto next = getActiveLineView()->next();

    if (next)
    {
        next->setActive();
        mCurrentWindowNode = next->parent()->deepestActive();
    }
}

void MainView::Impl::activeTablineUp()
{
    mActiveTabline = clamp(mActiveTabline - 1, 0, mCurrentWindowNode->depth());
}

void MainView::Impl::activeTablineDown()
{
    mActiveTabline = clamp(mActiveTabline + 1, 0, mCurrentWindowNode->depth());
}

WindowNodes::iterator MainView::Impl::getWindowNodeIterator(WindowNode& node, WindowNodes& nodes)
{
    for (auto childIt = nodes.begin(); childIt != nodes.end(); ++childIt)
    {
        if (*childIt == &node)
        {
            return childIt;
        }
    }
    return nodes.end();
}

}  // namespace core

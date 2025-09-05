#define LOG_HEADER "ui::MainView"
#include "main_view.hpp"

#include <cstdint>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include "core/alias.hpp"
#include "core/assert.hpp"
#include "core/command.hpp"
#include "core/context.hpp"
#include "core/logger.hpp"
#include "core/message_line.hpp"
#include "core/mode.hpp"
#include "core/variable.hpp"
#include "core/view.hpp"
#include "core/views.hpp"
#include "sys/system.hpp"
#include "ui/config.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/ui_component.hpp"
#include "ui/view.hpp"
#include "utils/buffer.hpp"
#include "utils/format.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

constexpr size_t MAX_LINES_COPIED = 2000;

struct MainView::Impl final
{
    Impl(const Config& config);

    bool isViewLoaded() const;
    void scrollLeft(core::Context& context);
    void scrollRight(core::Context& context);
    void scrollLineDown(core::Context& context);
    void scrollLineUp(core::Context& context);
    void scrollPageDown(core::Context& context);
    void scrollPageUp(core::Context& context);
    void scrollToEnd(core::Context& context);
    void scrollTo(long lineNumber, core::Context& context);

    void reload(Ftxui& ui, core::Context& context);
    ViewNodePtr createView(std::string name, core::ViewId viewDataId, ViewNode* parentPtr);
    void removeView(ViewNode& view, core::Context& context);
    void removeViewChildren(ViewNode& view, core::Context& context);
    ViewNodes::iterator getViewIterator(ViewNode& view, ViewNodes& viewNodes);

    const char* activeFileName(core::Context& context) const;

    ViewNode* getActiveLineView();
    void activeTablineLeft();
    void activeTablineRight();
    void activeTablineUp();
    void activeTablineDown();

    Element render(core::Context& context);
    Elements renderTablines();

    bool handleEvent(const Event& event, Ftxui& ui, core::Context& context);

    ViewNode         root;
    ftxui::Component placeholder;
    View*            currentView;
    const Config&    config;
    int              activeTabline;
    int              currentHeight;
    EventHandlers    eventHandlers;

private:
    void resize(Ftxui& ui, core::Context& context);
    void escape();
    void yank(Ftxui& ui, core::Context& context);
    void selectionModeToggle(core::Context& context);
    void cursorAlign();
    void selectionUpdate();
};

#define RETURN_TRUE(...) ({ __VA_ARGS__; return true; })

MainView::Impl::Impl(const Config& cfg)
    : root(ViewNode::Type::group, "main")
    , placeholder(Container::Vertical({}))
    , currentView(nullptr)
    , config(cfg)
    , activeTabline(0)
    , eventHandlers{
        {Event::PageUp,         [this](auto&, auto& context){ RETURN_TRUE(scrollPageUp(context)); }},
        {Event::PageDown,       [this](auto&, auto& context){ RETURN_TRUE(scrollPageDown(context)); }},
        {Event::ArrowDown,      [this](auto&, auto& context){ RETURN_TRUE(scrollLineDown(context)); }},
        {Event::ArrowUp,        [this](auto&, auto& context){ RETURN_TRUE(scrollLineUp(context)); }},
        {Event::ArrowLeft,      [this](auto&, auto& context){ RETURN_TRUE(scrollLeft(context)); }},
        {Event::ArrowRight,     [this](auto&, auto& context){ RETURN_TRUE(scrollRight(context)); }},
        {Event::ArrowLeftCtrl,  [this](auto&, auto&){ RETURN_TRUE(activeTablineLeft()); }},
        {Event::ArrowRightCtrl, [this](auto&, auto&){ RETURN_TRUE(activeTablineRight()); }},
        {Event::ArrowUpCtrl,    [this](auto&, auto&){ RETURN_TRUE(activeTablineUp()); }},
        {Event::ArrowDownCtrl,  [this](auto&, auto&){ RETURN_TRUE(activeTablineDown()); }},
        {Event::Resize,         [this](auto& ui, auto& context){ RETURN_TRUE(resize(ui, context)); }},
        {Event::Escape,         [this](auto&, auto&){ RETURN_TRUE(escape()); }},
        {Event::y,              [this](auto& ui, auto& context){ RETURN_TRUE(yank(ui, context)); }},
        {Event::v,              [this](auto&, auto& context){ RETURN_TRUE(selectionModeToggle(context)); }},
    }
{
}

bool MainView::Impl::isViewLoaded() const
{
    return currentView and currentView->loaded;
}

constexpr static uint32_t u(char c)
{
    return uint32_t(uint8_t(c));
}

constexpr static uint32_t u(const std::string_view& c)
{
    if ((c[0] & 0b1000'0000) == 0b0000'0000) [[likely]]
    {
        return u(c[0]);
    }
    else if ((c[0] & 0b1110'0000) == 0b1100'0000 and c.size() > 1)
    {
        return u(c[0]) | (u(c[1]) << 8);
    }
    else if ((c[0] & 0b1111'0000) == 0b1110'0000 and c.size() > 2)
    {
        return u(c[0]) | (u(c[1]) << 8) | (u(c[2]) << 16);
    }
    else if ((c[0] & 0b1111'1000) == 0b1111'0000 and c.size() > 3)
    {
        return u(c[0]) | (u(c[1]) << 8) | (u(c[2]) << 16) | (u(c[3]) << 24);
    }
    else
    {
        return u(c[0]);
    }
}

static Glyphs getGlyphs(std::string_view line, const Config& config)
{
    Glyphs glyphs;
    glyphs.reserve(line.size());

    const auto lineStart = line.data();

    while (not line.empty())
    {
        if ((line[0] & 0b1000'0000) == 0b0000'0000) [[likely]]
        {
            switch (line[0])
            {
                case '\t':
                {
                    Glyph glyph{
                        .width = uint8_t(config.tabWidth),
                        .special = 1,
                        .offset = uint32_t(line.data() - lineStart),
                        .characters = {u(config.tabChar)},
                    };
                    if (config.tabWidth > 1)
                    {
                        for (uint8_t i = 1; i < config.tabWidth - 1; ++i)
                        {
                            glyph.characters[i] = ' ';
                        }
                    }
                    glyphs.emplace_back(std::move(glyph));
                    break;
                }
                case 0 ... '\b':
                case '\n' ... 0x1f:
                    glyphs.emplace_back(
                        Glyph{
                            .width = 2,
                            .special = 1,
                            .offset = uint32_t(line.data() - lineStart),
                            .characters = {'^', uint32_t(line[0] + 0x40)}
                        });
                    break;
                default:
                    glyphs.emplace_back(
                        Glyph{
                            .width = 1,
                            .special = 0,
                            .offset = uint32_t(line.data() - lineStart),
                            .characters = {u(line[0])}
                        });
                    break;
            }
            line.remove_prefix(1);
        }
        else if ((line[0] & 0b1110'0000) == 0b1100'0000 and line.size() > 1)
        {
            glyphs.emplace_back(
                Glyph{
                    .width = 1,
                    .special = 0,
                    .offset = uint32_t(line.data() - lineStart),
                    .characters = {u(line[0]) | (u(line[1]) << 8)}
                });
            line.remove_prefix(2);
        }
        else if ((line[0] & 0b1111'0000) == 0b1110'0000 and line.size() > 2)
        {
            glyphs.emplace_back(
                Glyph{
                    .width = 1,
                    .special = 0,
                    .offset = uint32_t(line.data() - lineStart),
                    .characters = {u(line[0]) | (u(line[1]) << 8) | (u(line[2]) << 16)}
                });
            line.remove_prefix(3);
        }
        else if ((line[0] & 0b1111'1000) == 0b1111'0000 and line.size() > 3)
        {
            glyphs.emplace_back(
                Glyph{
                    .width = 1,
                    .special = 0,
                    .offset = uint32_t(line.data() - lineStart),
                    .characters = {u(line[0]) | (u(line[1]) << 8) | (u(line[2]) << 16) | (u(line[3]) << 24)}
                });
            line.remove_prefix(4);
        }
        else
        {
            constexpr static const char hex[] = "0123456789abcdef";
            glyphs.emplace_back(
                Glyph{
                    .width = 4,
                    .special = 1,
                    .offset = uint32_t(line.data() - lineStart),
                    .characters = {
                        '<',
                        uint32_t(hex[u(line[0]) >> 4]),
                        uint32_t(hex[u(line[0]) & 0xf]),
                        '>'
                    }
                });
            line.remove_prefix(1);
        }
    }
    return glyphs;
}

static Line getLine(View& view, size_t lineIndex, core::Context& context)
{
    auto viewData = core::getView(view.dataId, context);

    if (not viewData) [[unlikely]]
    {
        return {};
    }

    auto result = viewData->readLine(lineIndex);

    if (not result) [[unlikely]]
    {
        return {};
    }

    const auto& config = context.ui->get<Ftxui>().config;

    const auto& data = result.value();

    auto glyphs = getGlyphs(data, config);

    Line line{
        .lineNumber = lineIndex,
        .absoluteLineNumber = viewData->absoluteLineNumber(lineIndex),
        .glyphs = std::move(glyphs),
    };

    if (data.find("ERR") != std::string::npos)
    {
        line.segments.emplace_back(line.glyphs, Color::Red);
    }
    else if (data.find("WRN") != std::string::npos)
    {
        line.segments.emplace_back(line.glyphs, Color::Yellow);
    }
    else if (data.find("DBG") != std::string::npos)
    {
        line.segments.emplace_back(line.glyphs, Color::Palette256(245));
    }
    else
    {
        line.segments.emplace_back(line.glyphs, Color());
    }

    return line;
}

static void reloadLines(View& view, core::Context& context)
{
    view.ringBuffer.clear();
    for (size_t i = view.yoffset; i < view.yoffset + view.viewHeight; ++i)
    {
        view.ringBuffer.pushBack(getLine(view, i, context));
    }
}

void MainView::Impl::scrollLeft(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (view.xcurrent == 0)
    {
        if (view.ycurrent > 0)
        {
            scrollLineUp(context);
            view.xcurrent = view.ringBuffer[view.ycurrent].glyphs.size();
        }
        return;
    }

    view.xcurrent--;
}

void MainView::Impl::scrollRight(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    const auto& line = view.ringBuffer[view.ycurrent];

    if (view.xcurrent == line.glyphs.size())
    {
        if (view.ycurrent < view.viewHeight - 1)
        {
            view.xcurrent = 0;
            scrollLineDown(context);
        }
        return;
    }

    view.xcurrent++;
}

void MainView::Impl::scrollLineDown(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (view.ringBuffer.size() == 0)
    {
        return;
    }

    if (view.ycurrent < view.viewHeight - 1)
    {
        ++view.ycurrent;
    }

    if (view.ycurrent >= view.viewHeight - 4)
    {
        if (view.yoffset + view.viewHeight >= view.lineCount)
        {
            cursorAlign();
            selectionUpdate();
            return;
        }

        view.ycurrent = view.viewHeight - 4;

        view.yoffset++;
        auto line = getLine(view, view.yoffset + view.viewHeight - 1, context);
        view.ringBuffer.pushBack(std::move(line));
    }

    cursorAlign();
    selectionUpdate();

    return;
}

void MainView::Impl::scrollLineUp(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (view.ycurrent > 0)
    {
        view.ycurrent--;
    }

    if (view.ycurrent < 3)
    {
        if (view.yoffset == 0)
        {
            cursorAlign();
            selectionUpdate();
            return;
        }

        view.ycurrent = 3;

        view.yoffset--;
        view.ringBuffer.pushFront(getLine(view, view.yoffset, context));
    }

    cursorAlign();
    selectionUpdate();
}

void MainView::Impl::scrollPageDown(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (view.ringBuffer.size() == 0)
    {
        return;
    }

    if (view.yoffset + view.viewHeight >= view.lineCount)
    {
        return;
    }

    view.yoffset = (view.yoffset + view.viewHeight)
        | utils::clamp(0ul, view.lineCount - view.viewHeight);

    cursorAlign();
    selectionUpdate();

    reloadLines(view, context);
}

void MainView::Impl::scrollPageUp(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (view.yoffset == 0)
    {
        return;
    }

    view.yoffset -= view.viewHeight;

    if (static_cast<long>(view.yoffset) < 0)
    {
        view.yoffset = 0;
    }

    cursorAlign();
    selectionUpdate();

    reloadLines(view, context);
}

void MainView::Impl::scrollToEnd(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    const auto lastViewLine = view.lineCount - view.viewHeight;

    if (view.yoffset >= lastViewLine)
    {
        return;
    }

    view.yoffset = lastViewLine;
    view.ycurrent = view.viewHeight - 1;

    cursorAlign();
    selectionUpdate();

    reloadLines(view, context);
}

void MainView::Impl::scrollTo(long lineNumber, core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    auto& view = *currentView;

    if (lineNumber == -1)
    {
        scrollToEnd(context);
        return;
    }

    lineNumber = (size_t)lineNumber | utils::clamp(0ul, view.lineCount - view.viewHeight);
    view.yoffset = lineNumber;
    view.ycurrent = 0;

    cursorAlign();
    selectionUpdate();
    reloadLines(view, context);
}

void MainView::Impl::reload(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    root.forEachRecursive(
        [&ui, &context](ViewNode& view)
        {
            if (view.type() == ViewNode::Type::view)
            {
                reloadView(view.cast<View>(), ui, context);
            }
        });
}

ViewNodePtr MainView::Impl::createView(std::string name, core::ViewId viewDataId, ViewNode* parentPtr)
{
    auto& parent = parentPtr
        ? *parentPtr
        : root;

    auto groupPtr = ViewNode::createGroup(std::move(name));
    auto basePtr = View::create("base", viewDataId, config);

    auto& group = parent
        .addChild(groupPtr)
        .setActive();

    if (not parentPtr)
    {
        group.depth(0);
        activeTabline = 0;
    }

    currentView = group
        .addChild(basePtr)
        .setActive()
        .ptrCast<View>();

    return groupPtr;
}

void MainView::Impl::removeView(ViewNode& view, core::Context& context)
{
    assert(view.type() == ViewNode::Type::group, utils::format("View {}:{} type is {}", view.name(), &view, view.type()));
    assert(view.parent(), utils::format("View {}:{} has no parent", view.name(), &view));

    auto& children = view.parent()->children();
    auto viewIt = getViewIterator(view, children);

    assert(viewIt != children.end(), utils::format("View {}:{} cannot be found in parent's children", view.name(), &view));

    removeViewChildren(view, context);

    logger.debug() << "removing " << view.name();

    children.erase(viewIt);

    auto nextIt = children.begin();

    if (nextIt != children.end())
    {
        auto& viewNode = **nextIt;
        viewNode.setActive();
        if (viewNode.type() == ViewNode::Type::view)
        {
            currentView = viewNode.ptrCast<View>();
        }
        else
        {
            currentView = viewNode.deepestActive()->ptrCast<View>();
        }
    }
    else
    {
        currentView = nullptr;
    }
}

void MainView::Impl::removeViewChildren(ViewNode& view, core::Context& context)
{
    auto& children = view.children();

    for (auto childIt = children.rbegin(); childIt != children.rend(); ++childIt)
    {
        auto& childViewNode = **childIt;

        if (&childViewNode == currentView)
        {
            currentView = nullptr;
        }

        removeViewChildren(childViewNode, context);

        logger.debug() << "removing " << childViewNode.name() << "; type: " << (int)childViewNode.type();

        if (childViewNode.type() == ViewNode::Type::view)
        {
            auto& view = childViewNode.cast<View>();
            logger.debug() << "freeing view data " << view.dataId.index;
            context.views.free(view.dataId);
        }
    }

    children.clear();
}

ViewNodes::iterator MainView::Impl::getViewIterator(ViewNode& view, ViewNodes& viewNodes)
{
    for (auto childIt = viewNodes.begin(); childIt != viewNodes.end(); ++childIt)
    {
        if (childIt->get() == &view)
        {
            return childIt;
        }
    }
    return viewNodes.end();
}

const char* MainView::Impl::activeFileName(core::Context& context) const
{
    if (not currentView)
    {
        return "[No Name]";
    }

    if (not currentView->loaded)
    {
        return "[Loading]";
    }

    auto viewData = core::getView(currentView->dataId, context);

    if (not viewData) [[unlikely]]
    {
        return "[Closed]";
    }

    return viewData->filePath().c_str();
}

static Element wrapActiveLineIf(Element line, bool condition)
{
    if (not condition)
    {
        return line;
    }
    return hbox({
        line,
        text("")
            | color(Palette::TabLine::inactiveLineBg)
            | bgcolor(Palette::TabLine::activeLineMarker),
        text("")
            | color(Palette::TabLine::activeLineMarker)
            | bgcolor(Palette::TabLine::activeLineBg)
            | xflex,
    });
}

Elements MainView::Impl::renderTablines()
{
    Elements vertical;
    vertical.reserve(5);

    int index = 0;

    vertical.push_back(
        wrapActiveLineIf(
            root.renderTabline(),
            activeTabline == index++));

    auto view = root.activeChild();

    while (view->activeChild())
    {
        vertical.push_back(
            wrapActiveLineIf(
                view->renderTabline(),
                activeTabline == index++));

        view = view->activeChild();
    }

    return vertical;
}

ViewNode* MainView::Impl::getActiveLineView()
{
    auto view = root.activeChild();

    while (view->activeChild())
    {
        if (view->depth() == activeTabline)
        {
            break;
        }
        view = view->activeChild();
    }

    return view;
}

void MainView::Impl::activeTablineLeft()
{
    auto prev = getActiveLineView()->prev();

    if (prev)
    {
        prev->setActive();
        currentView = prev->parent()->deepestActive()->ptrCast<View>();
    }
}

void MainView::Impl::activeTablineRight()
{
    auto next = getActiveLineView()->next();

    if (next)
    {
        next->setActive();
        currentView = next->parent()->deepestActive()->ptrCast<View>();
    }
}

void MainView::Impl::activeTablineUp()
{
    activeTabline = (activeTabline - 1) | utils::clamp(0, currentView->depth());
}

void MainView::Impl::activeTablineDown()
{
    activeTabline = (activeTabline + 1) | utils::clamp(0, currentView->depth());
}

Element MainView::Impl::render(core::Context& context)
{
    if (not currentView) [[unlikely]]
    {
        return vbox({
            text("Hello!") | center,
            text("Type :files<Enter> to open file picker") | center,
            text("Alternatively, type :e <file><Enter> to open given file") | center,
        }) | center | flex;
    }

    auto vertical = renderTablines();

    if (currentView->loaded) [[likely]]
    {
        auto viewData = core::getView(currentView->dataId, context);

        if (not viewData) [[unlikely]]
        {
            vertical.push_back(vbox({text("Closed") | center}) | center | flex);
        }
        else
        {
            vertical.push_back(currentView->viewRenderer);
        }
    }
    else
    {
        vertical.push_back(vbox({text("Loading...") | center}) | center | flex);
    }

    return vbox(std::move(vertical)) | flex;
}

bool MainView::Impl::handleEvent(const Event& event, Ftxui& ui, core::Context& context)
{
    auto result = eventHandlers.handleEvent(event, ui, context);

    if (not result)
    {
        return false;
    }

    return result.value();
}

void MainView::Impl::resize(Ftxui& ui, core::Context& context)
{
    reload(ui, context);
}

void MainView::Impl::escape()
{
    if (currentView)
    {
        currentView->selectionMode = false;
    }
}

void MainView::Impl::yank(Ftxui&, core::Context& context)
{
    if (not isViewLoaded() or not currentView->selectionMode)
    {
        return;
    }

    auto lineCount = currentView->selectionEnd - currentView->selectionStart + 1;

    if (lineCount > MAX_LINES_COPIED)
    {
        context.messageLine.error() << "cannot yank more than " << MAX_LINES_COPIED << " lines";
        return;
    }

    utils::Buffer buf;

    auto viewData = core::getView(currentView->dataId, context);

    if (not viewData) [[unlikely]]
    {
        return;
    }

    for (auto i = currentView->selectionStart; i <= currentView->selectionEnd; ++i)
    {
        if (const auto result = viewData->readLine(i))
        {
            buf << result.value() << '\n';
        }
    }

    sys::copyToClipboard(buf.str());

    currentView->selectionMode = false;
    core::switchMode(core::Mode::normal, context);

    context.messageLine.info() << lineCount << " lines copied to clipboard";
}

void MainView::Impl::selectionModeToggle(core::Context& context)
{
    if (not isViewLoaded())
    {
        return;
    }

    if ((currentView->selectionMode ^= true))
    {
        core::switchMode(core::Mode::visual, context);
        context.mode = core::Mode::visual;
        currentView->selectionPivot
            = currentView->selectionEnd
            = currentView->selectionStart
            = currentView->ycurrent + currentView->yoffset;
    }
    else
    {
        core::switchMode(core::Mode::normal, context);
    }

    return;
}

void MainView::Impl::cursorAlign()
{
    const auto& currentLine = currentView->ringBuffer[currentView->ycurrent];
    currentView->xcurrent = currentView->xcurrent | utils::clamp(0ul, currentLine.glyphs.size());
}

void MainView::Impl::selectionUpdate()
{
    if (not currentView->selectionMode)
    {
        return;
    }

    auto absoluteIndex = currentView->ycurrent + currentView->yoffset;

    if (absoluteIndex > currentView->selectionPivot)
    {
        currentView->selectionStart = currentView->selectionPivot;
        currentView->selectionEnd = absoluteIndex;
    }
    else if (absoluteIndex < currentView->selectionPivot)
    {
        currentView->selectionStart = absoluteIndex;
        currentView->selectionEnd = currentView->selectionPivot;
    }
    else
    {
        currentView->selectionEnd = currentView->selectionStart = absoluteIndex;
    }
}

MainView::MainView(const Config& config)
    : UIComponent(UIComponent::mainView)
    , mPimpl(new Impl(config))
{
}

MainView::~MainView()
{
    delete mPimpl;
}

void MainView::takeFocus()
{
}

bool MainView::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    return mPimpl->handleEvent(event, ui, context);
}

Element MainView::render(core::Context& context)
{
    return mPimpl->render(context);
}

void MainView::reload(Ftxui& ui, core::Context& context)
{
    mPimpl->reload(ui, context);
}

ViewNodePtr MainView::createView(std::string name, core::ViewId viewDataId, ViewNode* parentPtr)
{
    return mPimpl->createView(std::move(name), viewDataId, parentPtr);
}

void MainView::removeView(ViewNode& view, core::Context& context)
{
    mPimpl->removeView(view, context);
}

void MainView::scrollTo(Ftxui&, long lineNumber, core::Context& context)
{
    mPimpl->scrollTo(lineNumber, context);
}

const char* MainView::activeFileName(core::Context& context) const
{
    return mPimpl->activeFileName(context);
}

bool MainView::isViewLoaded() const
{
    return mPimpl->isViewLoaded();
}

View* MainView::currentView()
{
    return mPimpl->currentView;
}

ViewNode& MainView::root()
{
    return mPimpl->root;
}

MainView::operator ftxui::Component&()
{
    return mPimpl->placeholder;
}

static size_t getAvailableViewHeight(Ftxui& ui, View& view)
{
    return static_cast<size_t>(ui.terminalSize.dimy)
        - 1 // command line
        - 1 // status line
        - (view.depth() + 1);
}

void reloadView(View& view, Ftxui& ui, core::Context& context)
{
    auto viewData = core::getView(view.dataId, context);

    if (not viewData) [[unlikely]]
    {
        return;
    }

    view.lineCount = viewData->lineCount();
    view.viewHeight = std::min(getAvailableViewHeight(ui, view), view.lineCount);
    view.lineNrDigits = utils::numberOfDigits(viewData->fileLineCount());
    view.ringBuffer = RingBuffer(view.viewHeight);
    view.yoffset = view.yoffset | utils::clamp(0ul, viewData->lineCount() - view.viewHeight);
    view.ycurrent = view.ycurrent | utils::clamp(0ul, view.viewHeight - 1);

    reloadLines(view, context);
}

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return ui.config.showLineNumbers;
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();
        ui.config.showLineNumbers = value.boolean;
        ui.mainView.reload(ui, context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(absoluteLineNumbers, boolean, "Print file absolute line numbers")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return ui.config.absoluteLineNumbers;
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();
        ui.config.absoluteLineNumbers = value.boolean;
        ui.mainView.reload(ui, context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(tabChar, string, "Tab character")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return &ui.config.tabChar;
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();
        ui.config.tabChar = *value.string;
        ui.mainView.reload(ui, context);
        return true;
    }
}

DEFINE_READWRITE_VARIABLE(tabWidth, integer, "Tab width")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return long(ui.config.tabWidth);
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();
        if (value.integer > 0xff)
        {
            context.messageLine.error() << "Invalid tab width: " << value.integer;
            return false;
        }
        ui.config.tabWidth = value.integer & 0xff;
        ui.mainView.reload(ui, context);
        return true;
    }
}

DEFINE_READONLY_VARIABLE(path, string, "Path to the opened file")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        if (not ui.mainView.isViewLoaded())
        {
            return nullptr;
        }

        auto viewData = core::getView(ui.mainView.currentView()->dataId, context);

        if (not viewData)
        {
            return nullptr;
        }

        return &viewData->filePath();
    }
}

DEFINE_COMMAND(quit)
{
    HELP() = "close current view or quit the program";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto& ui = context.ui->get<Ftxui>();

        auto currentView = ui.mainView.currentView();

        if (not currentView)
        {
            ui.quit(context);
            return true;
        }

        ViewNode* viewToClose = currentView->isBase()
            ? currentView->parent()
            : currentView;

        ui.mainView.removeView(*viewToClose, context);

        return true;
    }
}

DEFINE_ALIAS(q, quit);

DEFINE_COMMAND(filter)
{
    HELP() = "filter current view";

    FLAGS()
    {
        return {};
    }

    ARGUMENTS()
    {
        return {};
    }

    EXECUTOR()
    {
        auto parentViewId = context.ui->getCurrentView();

        auto parentView = getView(parentViewId, context);

        if (not parentView) [[unlikely]]
        {
            context.messageLine.error() << "No view loaded yet";
            return false;
        }

        utils::Buffer buf;

        auto& ui = context.ui->get<Ftxui>();

        if (not ui.mainView.currentView()->selectionMode)
        {
            context.messageLine.error() << "Nothing selected";
            return false;
        }

        size_t start = ui.mainView.currentView()->selectionStart;
        size_t end = ui.mainView.currentView()->selectionEnd;

        buf << '<' << start << '-' << end << '>';

        auto [newView, newViewId] = context.views.allocate();

        auto uiView = context.ui->createView(buf.str(), newViewId, core::Parent::currentView, context);

        if (uiView.expired()) [[unlikely]]
        {
            context.messageLine.error() << "failed to create UI view";
            context.views.free(newViewId);
            return false;
        }

        newView.filter(
            start,
            end,
            parentViewId,
            context,
            [uiView, newViewId, &context](core::TimeOrError result)
            {
                if (result)
                {
                    auto newView = getView(newViewId, context);

                    if (newView)
                    {
                        context.messageLine.info()
                            << "filtered " << newView->lineCount() << " lines; took "
                            << (result.value() | utils::precision(3)) << " s";

                        context.ui->onViewDataLoaded(uiView, context);
                    }
                }
                else if (context.running)
                {
                    context.messageLine.error() << "Error filtering view: " << result.error();
                    context.ui->removeView(uiView, context);
                }
            });

        return true;
    }
}

}  // namespace ui

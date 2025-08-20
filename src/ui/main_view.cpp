#include "main_view.hpp"

#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/context.hpp"
#include "core/mode.hpp"
#include "core/variable.hpp"
#include "sys/system.hpp"
#include "ui/event_handler.hpp"
#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "ui/ui_component.hpp"
#include "ui/view.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

constexpr size_t MAX_LINES_COPIED = 2000;

struct MainView::Impl final
{
    Impl();

    bool isViewLoaded() const;
    bool scrollLeft(core::Context& context);
    bool scrollRight(core::Context& context);
    bool scrollLineDown(core::Context& context);
    bool scrollLineUp(core::Context& context);
    bool scrollPageDown(core::Context& context);
    bool scrollPageUp(core::Context& context);
    bool scrollToEnd(core::Context& context);
    void scrollTo(long lineNumber, core::Context& context);

    void reload(Ftxui& ui, core::Context& context);
    ViewNode& createView(std::string name, ViewNode* parentPtr);

    ViewNode* getActiveLineView();
    bool activeTablineLeft();
    bool activeTablineRight();
    bool activeTablineUp();
    bool activeTablineDown();

    Element render();
    Elements renderTablines();

    bool handleEvent(const Event& event, Ftxui& ui, core::Context& context);

    ViewNode          root;
    ftxui::Component  lines;
    View*             currentView;
    int               activeTabline;
    int               currentHeight;
    EventHandlers     eventHandlers;

private:
    bool resize(Ftxui& ui, core::Context& context);
    bool escape();
    bool yank(Ftxui& ui, core::Context& context);
    bool selectionModeToggle(core::Context& context);
    void selectionUpdate();
};

MainView::Impl::Impl()
    : root(ViewNode::Type::group, "main")
    , lines(Container::Vertical({}))
    , currentView(nullptr)
    , activeTabline(0)
    , eventHandlers{
        {Event::PageUp,         [this](auto&, auto& context){ return scrollPageUp(context); }},
        {Event::PageDown,       [this](auto&, auto& context){ return scrollPageDown(context); }},
        {Event::ArrowDown,      [this](auto&, auto& context){ return scrollLineDown(context); }},
        {Event::ArrowUp,        [this](auto&, auto& context){ return scrollLineUp(context); }},
        {Event::ArrowLeft,      [this](auto&, auto& context){ return scrollLeft(context); }},
        {Event::ArrowRight,     [this](auto&, auto& context){ return scrollRight(context); }},
        {Event::ArrowLeftCtrl,  [this](auto&, auto&){ return activeTablineLeft(); }},
        {Event::ArrowRightCtrl, [this](auto&, auto&){ return activeTablineRight(); }},
        {Event::ArrowUpCtrl,    [this](auto&, auto&){ return activeTablineUp(); }},
        {Event::ArrowDownCtrl,  [this](auto&, auto&){ return activeTablineDown(); }},
        {Event::Resize,         [this](auto& ui, auto& context){ return resize(ui, context); }},
        {Event::Escape,         [this](auto&, auto&){ return escape(); }},
        {Event::y,              [this](auto& ui, auto& context){ return yank(ui, context); }},
        {Event::v,              [this](auto&, auto& context){ return selectionModeToggle(context); }},
    }
{
}

bool MainView::Impl::isViewLoaded() const
{
    return currentView and currentView->file;
}

bool MainView::Impl::scrollLeft(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    if (view.xoffset == 0)
    {
        return true;
    }

    view.xoffset--;
    reloadLines(view, context);
    return true;
}

bool MainView::Impl::scrollRight(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    view.xoffset++;
    reloadLines(view, context);
    return true;
}

bool MainView::Impl::scrollLineDown(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.ycurrent < view.viewHeight - 1)
    {
        ++view.ycurrent;
    }

    if (view.ycurrent >= view.viewHeight - 4)
    {
        if (view.yoffset + view.viewHeight >= view.lineCount)
        {
            selectionUpdate();
            return true;
        }

        view.ycurrent = view.viewHeight - 4;

        view.yoffset++;
        auto line = getLine(view, view.yoffset + view.viewHeight - 1, context);
        view.ringBuffer.pushBack(std::move(line));
    }

    selectionUpdate();

    return true;
}

bool MainView::Impl::scrollLineUp(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
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
            selectionUpdate();
            return true;
        }

        view.ycurrent = 3;

        view.yoffset--;
        view.ringBuffer.pushFront(getLine(view, view.yoffset, context));
    }

    selectionUpdate();

    return true;
}

bool MainView::Impl::scrollPageDown(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    if (view.ringBuffer.size() == 0)
    {
        return true;
    }

    if (view.yoffset + view.viewHeight >= view.lineCount)
    {
        return true;
    }

    view.yoffset = (view.yoffset + view.viewHeight)
        | utils::clamp(0ul, view.lineCount - view.viewHeight);

    selectionUpdate();

    reloadLines(view, context);

    return true;
}

bool MainView::Impl::scrollPageUp(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    if (view.yoffset == 0)
    {
        return true;
    }

    view.yoffset -= view.viewHeight;

    if (static_cast<long>(view.yoffset) < 0)
    {
        view.yoffset = 0;
    }

    selectionUpdate();

    reloadLines(view, context);

    return true;
}

bool MainView::Impl::scrollToEnd(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
    }

    auto& view = *currentView;

    const auto lastViewLine = view.lineCount - view.viewHeight;

    if (view.yoffset >= lastViewLine)
    {
        return true;
    }

    view.yoffset = lastViewLine;
    view.ycurrent = view.viewHeight - 1;

    selectionUpdate();

    reloadLines(view, context);

    return true;
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

ViewNode& MainView::Impl::createView(std::string name, ViewNode* parentPtr)
{
    auto& parent = parentPtr
        ? *parentPtr
        : root;

    auto& group = parent
        .addChild(ViewNode::createGroup(std::move(name)))
        .setActive();

    if (not parentPtr)
    {
        group.depth(0);
        activeTabline = 0;
    }

    currentView = group
        .addChild(View::create("base"))
        .setActive()
        .ptrCast<View>();

    return group;
}

static const MenuEntryOption tabOption{
    .transform =
        [](const EntryState& state)
        {
            auto string = ' ' + std::to_string(state.index) + ' ' + state.label + ' ';

            if (state.focused)
            {
                if (state.index != 0)
                {
                    return hbox({
                        text("")
                            | bgcolor(Palette::TabLine::activeBg)
                            | color(Palette::TabLine::activeFg),
                        text(std::move(string))
                            | bgcolor(Palette::TabLine::activeBg)
                            | color(Palette::TabLine::activeFg)
                            | bold,
                        text("")
                            | color(Palette::TabLine::activeBg)
                            | bgcolor(Palette::TabLine::separatorBg),
                    });
                }
                else
                {
                    return hbox({
                        text(std::move(string))
                            | bgcolor(Palette::TabLine::activeBg)
                            | color(Palette::TabLine::activeFg)
                            | bold,
                        text("")
                            | color(Palette::TabLine::activeBg)
                            | bgcolor(Palette::TabLine::activeFg),
                    });
                }
            }
            else
            {
                if (state.index != 0)
                {
                    return hbox({
                        text("")
                            | bgcolor(Palette::TabLine::inactiveBg)
                            | color(Palette::TabLine::separatorBg),
                        text(std::move(string))
                            | bgcolor(Palette::TabLine::inactiveBg),
                        text("")
                            | color(Palette::TabLine::inactiveBg)
                            | bgcolor(Palette::TabLine::separatorBg),
                    });
                }
                else
                {
                    return hbox({
                        text(std::move(string))
                            | bgcolor(Palette::TabLine::inactiveBg),
                        text("")
                            | color(Palette::TabLine::inactiveBg)
                            | bgcolor(Palette::TabLine::separatorBg),
                    });
                }
            }
        }
};

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

struct GetLines
{
    View& view;
};

GetLines getLines(View& view)
{
    return {.view = view};
}

static Elements operator|(LinesRingBuffer& buf, const GetLines& obj)
{
    Elements vec;
    size_t i = 0;
    buf.forEach(
        [&i, &obj, &vec](const auto& line)
        {
            auto& view = obj.view;
            auto absoluteIndex = i + view.yoffset;
            if (view.ycurrent == i)
            {
                vec.emplace_back(text(&line.line) | bgcolor(Palette::bg3));
            }
            else if (view.selectionMode and absoluteIndex >= view.selectionStart and absoluteIndex <= view.selectionEnd)
            {
                vec.emplace_back(text(&line.line) | bgcolor(Palette::bg2));
            }
            else
            {
                vec.emplace_back(text(&line.line));
            }
            ++i;
        });
    return vec;
}

static constexpr struct GetLineNumbers{} getLineNumbers;

static Elements operator|(LinesRingBuffer& buf, const GetLineNumbers&)
{
    Elements vec;
    buf.forEach(
        [&vec](const auto& line)
        {
            vec.emplace_back(text(&line.lineNumber) | xflex);
        });
    return vec;
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

bool MainView::Impl::activeTablineLeft()
{
    auto prev = getActiveLineView()->prev();

    if (prev)
    {
        prev->setActive();
        currentView = prev->parent()->deepestActive()->ptrCast<View>();
    }

    return true;
}

bool MainView::Impl::activeTablineRight()
{
    auto next = getActiveLineView()->next();

    if (next)
    {
        next->setActive();
        currentView = next->parent()->deepestActive()->ptrCast<View>();
    }

    return true;
}

bool MainView::Impl::activeTablineUp()
{
    activeTabline = (activeTabline - 1)
        | utils::clamp(0, currentView->depth());
    return true;
}

bool MainView::Impl::activeTablineDown()
{
    activeTabline = (activeTabline + 1)
        | utils::clamp(0, currentView->depth());
    return true;
}

Element MainView::Impl::render()
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

    if (currentView->file) [[likely]]
    {
        vertical.push_back(
            hbox(
                nonSelectableVbox(currentView->ringBuffer | getLineNumbers),
                vbox(currentView->ringBuffer | getLines(*currentView)) | xflex));
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

bool MainView::Impl::resize(Ftxui& ui, core::Context& context)
{
    reload(ui, context);
    return true;
}

bool MainView::Impl::escape()
{
    if (currentView)
    {
        currentView->selectionMode = false;
    }
    return true;
}

bool MainView::Impl::yank(Ftxui& ui, core::Context& context)
{
    if (not isViewLoaded() or not currentView->selectionMode)
    {
        return true;
    }

    auto lineCount = currentView->selectionEnd - currentView->selectionStart + 1;

    if (lineCount > MAX_LINES_COPIED)
    {
        ui << error << "cannot yank more than " << MAX_LINES_COPIED << " lines";
        return true;
    }

    std::stringstream ss;

    for (auto i = currentView->selectionStart; i <= currentView->selectionEnd; ++i)
    {
        auto lineIndex = i;
        if (not currentView->lines.empty())
        {
            lineIndex = currentView->lines[lineIndex];
        }
        ss << (*currentView->file)[lineIndex] << '\n';
    }

    sys::copyToClipboard(ss.str());

    currentView->selectionMode = false;
    core::switchMode(core::Mode::normal, context);

    ui << info << lineCount << " lines copied to clipboard";

    return true;
}

bool MainView::Impl::selectionModeToggle(core::Context& context)
{
    if (not isViewLoaded())
    {
        return true;
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

    return true;
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

MainView::MainView()
    : UIComponent(UIComponent::mainView)
    , pimpl_(new Impl)
{
}

MainView::~MainView()
{
    delete pimpl_;
}

void MainView::takeFocus()
{
    pimpl_->lines->TakeFocus();
}

bool MainView::handleEvent(const ftxui::Event& event, Ftxui& ui, core::Context& context)
{
    return pimpl_->handleEvent(event, ui, context);
}

Element MainView::render(core::Context&)
{
    return pimpl_->render();
}

void MainView::reload(Ftxui& ui, core::Context& context)
{
    pimpl_->reload(ui, context);
}

ViewNode& MainView::createView(std::string name, ViewNode* parentPtr)
{
    return pimpl_->createView(std::move(name), parentPtr);
}

void MainView::scrollTo(Ftxui&, long lineNumber, core::Context& context)
{
    pimpl_->scrollTo(lineNumber, context);
}

const char* MainView::activeFileName() const
{
    return pimpl_->currentView
        ? pimpl_->currentView->file
            ? pimpl_->currentView->file->path().c_str()
            : "loading..."
        : "[No Name]";
}

bool MainView::isViewLoaded() const
{
    return pimpl_->isViewLoaded();
}

View* MainView::currentView()
{
    return pimpl_->currentView;
}

ViewNode& MainView::root()
{
    return pimpl_->root;
}

MainView::operator ftxui::Component&()
{
    return pimpl_->lines;
}

DEFINE_READWRITE_VARIABLE(showLineNumbers, boolean, "Show line numbers on the left")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return ui.showLineNumbers;
    }

    WRITER()
    {
        auto& ui = context.ui->get<Ftxui>();
        ui.showLineNumbers = value.boolean;
        ui.mainView.reload(ui, context);
        return true;
    }
}

DEFINE_READONLY_VARIABLE(path, string, "Path to the opened file")
{
    READER()
    {
        auto& ui = context.ui->get<Ftxui>();
        return ui.mainView.isViewLoaded()
            ? &ui.mainView.currentView()->file->path()
            : nullptr;
    }
}

}  // namespace ui

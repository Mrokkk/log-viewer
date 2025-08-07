#include "view.hpp"

#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/colored_string.hpp>

#include "ui/ftxui.hpp"
#include "ui/palette.hpp"
#include "utils/math.hpp"

using namespace ftxui;

namespace ui
{

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

ViewNode::ViewNode(Type type, std::string name)
    : type_(type)
    , depth_(0)
    , name_(name)
    , parent_(nullptr)
    , activeChild_(nullptr)
    , tab_(MenuEntry(name, tabOption))
    , childrenTabs_(Container::Horizontal({}))
{
}

ViewNodePtr ViewNode::createLink(std::string name)
{
    return std::make_unique<ViewNode>(Type::link, std::move(name));
}

ftxui::Element ViewNode::renderTab() const
{
    return tab_->Render();
}

ftxui::Element ViewNode::renderTabline() const
{
    return childrenTabs_->Render();
}

ViewNode& ViewNode::addChild(ViewNodePtr child)
{
    auto& childRef = *child;
    child->parent_ = this;
    child->depth_ = depth_ + 1;

    childrenTabs_->Add(child->tab_);
    children_.emplace_back(std::move(child));
    return childRef;
}

ViewNode* ViewNode::childAt(unsigned index)
{
    if (index >= children_.size())
    {
        return nullptr;
    }

    auto it = std::next(children_.begin(), index);

    return it->get();
}

ViewNode& ViewNode::setActive()
{
    parent_->setActiveChild(*this);
    return *this;
}

void ViewNode::setActiveChild(ViewNode& node)
{
    for (auto& child : children_)
    {
        if (child.get() == &node)
        {
            childrenTabs_->SetActiveChild(child->tab_);
            activeChild_ = child.get();
            return;
        }
    }
    throw std::runtime_error("no such child!");
}

ViewNode* ViewNode::next()
{
    if (not parent_)
    {
        return nullptr;
    }

    for (auto it = parent_->children_.begin(); it != parent_->children_.end(); ++it)
    {
        if (it->get() == this)
        {
            return ++it == parent_->children_.end()
                ? nullptr
                : it->get();
        }
    }

    return nullptr;
}

ViewNode* ViewNode::prev()
{
    if (not parent_)
    {
        return nullptr;
    }

    for (auto it = parent_->children_.begin(); it != parent_->children_.end(); ++it)
    {
        if (it->get() == this)
        {
            return it == parent_->children_.begin()
                ? nullptr
                : (--it)->get();
        }
    }

    return nullptr;
}

ViewNode* ViewNode::deepestActive()
{
    auto child = activeChild_;

    if (not child)
    {
        return nullptr;
    }

    while (child->activeChild())
    {
        child = child->activeChild();
    }

    return child;
}

View::View(std::string name)
    : ViewNode(ViewNode::Type::view, std::move(name))
    , file(nullptr)
    , lineCount(0)
    , viewHeight(0)
    , yoffset(0)
    , xoffset(0)
    , lineNrDigits(0)
    , ringBuffer(0)
{
}

ViewPtr View::create(std::string name)
{
    return std::make_unique<View>(std::move(name));
}

std::string getLine(View& view, size_t lineIndex, core::Context& context)
{
    std::stringstream ss;

    auto& ui = context.ui->get<ui::Ftxui>();

    if (not view.lines.empty())
    {
        lineIndex = view.lines[lineIndex];
    }

    if (ui.showLineNumbers)
    {
        ss << std::setw(view.lineNrDigits + 1) << std::right
           << ColorWrapped(lineIndex, Palette::View::lineNumberFg)
           << ColorWrapped("│ ", Palette::View::lineNumberFg);
    }

    auto line = (*view.file)[lineIndex];
    auto xoffset = view.xoffset | utils::clamp(0ul, line.length());

    if (line.find("ERR") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Red);
    }
    else if (line.find("WRN") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Yellow);
    }
    else if (line.find("DBG") != std::string::npos)
    {
        ss << ColorWrapped(line.c_str() + xoffset, Color::Palette256(245));
    }
    else
    {
        ss << line.c_str() + xoffset;
    }

    return ss.str();
}

void reloadLines(View& view, core::Context& context)
{
    for (size_t i = view.yoffset; i < view.yoffset + view.viewHeight; ++i)
    {
        view.ringBuffer.pushBack(getLine(view, i, context));
    }
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
    view.viewHeight = std::min(getAvailableViewHeight(ui, view), view.lineCount);
    view.lineNrDigits = utils::numberOfDigits(view.file->lineCount());
    view.ringBuffer = utils::RingBuffer<std::string>(view.viewHeight);
    view.yoffset = view.yoffset | utils::clamp(0ul, view.lineCount - view.viewHeight);

    reloadLines(view, context);
}

bool isViewLoaded(Ftxui& ui)
{
    return ui.currentView and ui.currentView->file;
}

}  // namespace ui

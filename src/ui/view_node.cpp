#define LOG_HEADER "ui::ViewNode"
#include "view_node.hpp"

#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/palette.hpp"

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

ViewNode::~ViewNode()
{
    tab_->Detach();
}

ViewNodePtr ViewNode::createGroup(std::string name)
{
    return std::make_shared<ViewNode>(Type::group, std::move(name));
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
    std::abort();
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

}  // namespace ui

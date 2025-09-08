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
    : mType(type)
    , mDepth(0)
    , mName(name)
    , mParent(nullptr)
    , mActiveChild(nullptr)
    , mTab(MenuEntry(name, tabOption))
    , mChildrenTabs(Container::Horizontal({}))
{
}

ViewNode::~ViewNode()
{
    mTab->Detach();
}

ViewNodePtr ViewNode::createGroup(std::string name)
{
    return std::make_shared<ViewNode>(Type::group, std::move(name));
}

ftxui::Element ViewNode::renderTabline() const
{
    return mChildrenTabs->Render();
}

ViewNode& ViewNode::addChild(ViewNodePtr child)
{
    auto& childRef = *child;
    child->mParent = this;
    child->mDepth = mDepth + 1;

    mChildrenTabs->Add(child->mTab);
    mChildren.emplace_back(std::move(child));
    return childRef;
}

ViewNode* ViewNode::childAt(unsigned index)
{
    if (index >= mChildren.size())
    {
        return nullptr;
    }

    auto it = std::next(mChildren.begin(), index);

    return it->get();
}

ViewNode& ViewNode::setActive()
{
    mParent->setActiveChild(*this);
    return *this;
}

void ViewNode::setActiveChild(ViewNode& node)
{
    for (auto& child : mChildren)
    {
        if (child.get() == &node)
        {
            mChildrenTabs->SetActiveChild(child->mTab);
            mActiveChild = child.get();
            return;
        }
    }
    std::abort();
}

ViewNode* ViewNode::next()
{
    if (not mParent)
    {
        return nullptr;
    }

    for (auto it = mParent->mChildren.begin(); it != mParent->mChildren.end(); ++it)
    {
        if (it->get() == this)
        {
            return ++it == mParent->mChildren.end()
                ? nullptr
                : it->get();
        }
    }

    return nullptr;
}

ViewNode* ViewNode::prev()
{
    if (not mParent)
    {
        return nullptr;
    }

    for (auto it = mParent->mChildren.begin(); it != mParent->mChildren.end(); ++it)
    {
        if (it->get() == this)
        {
            return it == mParent->mChildren.begin()
                ? nullptr
                : (--it)->get();
        }
    }

    return nullptr;
}

ViewNode* ViewNode::deepestActive()
{
    auto child = mActiveChild;

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

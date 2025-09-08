#pragma once

#include <list>
#include <memory>

#include <ftxui/fwd.hpp>

#include "ui/fwd.hpp"
#include "utils/noncopyable.hpp"

namespace ui
{

struct View;
struct ViewNode;

using ViewNodePtr = std::shared_ptr<ViewNode>;
using ViewPtr = std::shared_ptr<View>;
using ViewNodeWeakPtr = std::weak_ptr<ViewNode>;
using ViewNodes = std::list<ViewNodePtr>;

struct ViewNode : utils::NonCopyable
{
    enum class Type
    {
        group,
        view,
    };

    ViewNode(Type type, std::string name);
    virtual ~ViewNode();

    static ViewNodePtr createGroup(std::string name);

    Type type() const
    {
        return mType;
    }

    std::string& name()
    {
        return mName;
    }

    const std::string& name() const
    {
        return mName;
    }

    ftxui::Element renderTabline() const;

    ViewNode& addChild(ViewNodePtr child);

    ViewNode* activeChild()
    {
        return mActiveChild;
    }

    ViewNode* parent()
    {
        return mParent;
    }

    ViewNode* childAt(unsigned index);

    ViewNodes& children()
    {
        return mChildren;
    }

    const ViewNodes& children() const
    {
        return mChildren;
    }

    ViewNode& setActive();

    void setActiveChild(ViewNode& node);

    template <typename T>
    T& cast()
    {
        return static_cast<T&>(*this);
    }

    template <typename T>
    T* ptrCast()
    {
        return static_cast<T*>(this);
    }

    ViewNode& base()
    {
        return *mChildren.front().get();
    }

    bool isBase() const
    {
        return mParent and this == &mParent->base();
    }

    ViewNode& depth(int i)
    {
        mDepth = i;
        return *this;
    }

    int depth() const
    {
        return mDepth;
    }

    ViewNode* next();
    ViewNode* prev();
    ViewNode* deepestActive();

    template <typename T>
    void forEachRecursive(T functor)
    {
        for (auto& child : mChildren)
        {
            functor(*child);
            child->forEachRecursive(functor);
        }
    }

private:
    Type             mType;
    int              mDepth;
    std::string      mName;
    ViewNodes        mChildren;
    ViewNode*        mParent;
    ViewNode*        mActiveChild;
    ftxui::Component mTab;
    ftxui::Component mChildrenTabs;
};

}  // namespace ui

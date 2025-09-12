#include "window_node.hpp"

#include "core/buffers.hpp"
#include "core/context.hpp"
#include "utils/unique_ptr.hpp"

namespace core
{

WindowNode::WindowNode(std::string name)
    : mType(Type::group)
    , mWindow{}
    , mDepth(0)
    , mName(name)
    , mParent(nullptr)
    , mActiveChild(nullptr)
{
}

WindowNode::WindowNode(std::string name, BufferId bufferId, Context& context)
    : mType(Type::window)
    , mWindow(bufferId, context)
    , mDepth(0)
    , mName(name)
    , mParent(nullptr)
    , mActiveChild(nullptr)
{
}

WindowNode::~WindowNode()
{
    if (mType == Type::group)
    {
        // In group window children needs to be destroyed
        // in reverse order to destroy base as last one.
        // This is required to handle top window quit while
        // grep/filter is ongoing for some child
        while (not mChildren.empty())
        {
            mChildren.pop_back();
        }
    }
    else if (mType == Type::window and mWindow.initialized)
    {
        Context::instance().buffers.free(mWindow.bufferId);
    }
}

WindowNodePtr WindowNode::createGroup(std::string name)
{
    return utils::makeUnique<WindowNode>(std::move(name));
}

WindowNodePtr WindowNode::createWindow(std::string name, BufferId bufferId, Context& context)
{
    return utils::makeUnique<WindowNode>(std::move(name), bufferId, context);
}

std::string& WindowNode::name()
{
    return mName;
}

const std::string& WindowNode::name() const
{
    return mName;
}

WindowNode& WindowNode::addChild(WindowNodePtr child)
{
    child->mParent = this;
    child->mDepth = mDepth + 1;
    return *mChildren.emplace_back(std::move(child));
}

WindowNode* WindowNode::activeChild()
{
    return mActiveChild;
}

const WindowNode* WindowNode::activeChild() const
{
    return mActiveChild;
}

WindowNode* WindowNode::parent()
{
    return mParent;
}

WindowNode* WindowNode::childAt(unsigned index)
{
    if (index >= mChildren.size())
    {
        return nullptr;
    }

    auto it = std::next(mChildren.begin(), index);

    return it->get();
}

WindowNodes& WindowNode::children()
{
    return mChildren;
}

const WindowNodes& WindowNode::children() const
{
    return mChildren;
}

WindowNode& WindowNode::setActive()
{
    mParent->setActiveChild(*this);
    return *this;
}

void WindowNode::setActiveChild(WindowNode& node)
{
    for (auto& child : mChildren)
    {
        if (child == &node)
        {
            mActiveChild = child.get();
            return;
        }
    }
    std::abort();
}

WindowNode& WindowNode::base()
{
    return *mChildren.front();
}

bool WindowNode::isBase() const
{
    return mParent and this == &mParent->base();
}

WindowNode& WindowNode::depth(int i)
{
    mDepth = i;
    return *this;
}

int WindowNode::depth() const
{
    return mDepth;
}

WindowNode* WindowNode::next()
{
    if (not mParent)
    {
        return nullptr;
    }

    for (auto it = mParent->mChildren.begin(); it != mParent->mChildren.end(); ++it)
    {
        if (*it == this)
        {
            return ++it == mParent->mChildren.end()
                ? nullptr
                : it->get();
        }
    }

    return nullptr;

}

WindowNode* WindowNode::prev()
{
    if (not mParent)
    {
        return nullptr;
    }

    for (auto it = mParent->mChildren.begin(); it != mParent->mChildren.end(); ++it)
    {
        if (*it == this)
        {
            return it == mParent->mChildren.begin()
                ? nullptr
                : (--it)->get();
        }
    }

    return nullptr;
}

WindowNode* WindowNode::deepestActive()
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

Buffer* WindowNode::buffer()
{
    if (mType == Type::group or not mWindow.initialized) [[unlikely]]
    {
        return nullptr;
    }
    return getBuffer(mWindow.bufferId, Context::instance());
}

void WindowNode::forEachRecursive(const std::function<void(WindowNode&)>& functor)
{
    for (auto& child : mChildren)
    {
        functor(*child);
        child->forEachRecursive(functor);
    }
}

}  // namespace core

#pragma once

#include <list>
#include <string>

#include "core/buffers.hpp"
#include "core/fwd.hpp"
#include "core/window.hpp"
#include "utils/function_ref.hpp"
#include "utils/immobile.hpp"
#include "utils/unique_ptr.hpp"

namespace core
{

using WindowNodePtr = utils::UniquePtr<WindowNode>;
using WindowNodes = std::list<WindowNodePtr>;

struct WindowNode final : utils::Immobile
{
    enum class Type
    {
        group,
        window,
    };

    WindowNode(std::string name);
    WindowNode(std::string name, BufferId bufferId, Context& context);

    ~WindowNode();

    static WindowNodePtr createGroup(std::string name);
    static WindowNodePtr createWindow(std::string name, BufferId bufferId, Context& context);

    constexpr Type type() const { return mType; }
    constexpr bool loaded() const { return mWindow.loaded; }
    constexpr void loaded(bool loaded) { mWindow.loaded = loaded; }
    constexpr BufferId bufferId() const { return mWindow.bufferId; }

    constexpr auto& window() const { return mWindow; }
    constexpr auto& window()       { return mWindow; }

    std::string& name();

    const std::string& name() const;

    WindowNode& addChild(WindowNodePtr child);

    WindowNode* activeChild();
    const WindowNode* activeChild() const;

    WindowNode* parent();
    WindowNode* childAt(unsigned index);

    WindowNodes& children();

    const WindowNodes& children() const;

    WindowNode& setActive();

    void setActiveChild(WindowNode& node);

    WindowNode& base();

    bool isBase() const;

    WindowNode& depth(int i);

    int depth() const;

    WindowNode* next();
    WindowNode* prev();
    WindowNode* deepestActive();

    Buffer* buffer();

    using Visitor = utils::FunctionRef<void(WindowNode&)>;

    void forEachRecursive(const Visitor& functor);

private:
    Type        mType;
    Window      mWindow;
    int         mDepth;
    std::string mName;
    WindowNodes mChildren;
    WindowNode* mParent;
    WindowNode* mActiveChild;
};

}  // namespace core

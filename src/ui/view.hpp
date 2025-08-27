#pragma once

#include <list>
#include <memory>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/fwd.hpp"
#include "core/views.hpp"
#include "ui/fwd.hpp"
#include "utils/noncopyable.hpp"
#include "utils/ring_buffer.hpp"

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
        return type_;
    }

    std::string& name()
    {
        return name_;
    }

    const std::string& name() const
    {
        return name_;
    }

    ftxui::Element renderTabline() const;

    ViewNode& addChild(ViewNodePtr child);

    ViewNode* activeChild()
    {
        return activeChild_;
    }

    ViewNode* parent()
    {
        return parent_;
    }

    ViewNode* childAt(unsigned index);

    ViewNodes& children()
    {
        return children_;
    }

    const ViewNodes& children() const
    {
        return children_;
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
        return *children_.front().get();
    }

    bool isBase() const
    {
        return parent_ and this == &parent_->base();
    }

    ViewNode& depth(int i)
    {
        depth_ = i;
        return *this;
    }

    int depth() const
    {
        return depth_;
    }

    ViewNode* next();
    ViewNode* prev();
    ViewNode* deepestActive();

    template <typename T>
    void forEachRecursive(T functor)
    {
        for (auto& child : children_)
        {
            functor(*child);
            child->forEachRecursive(functor);
        }
    }

private:
    Type             type_;
    int              depth_;
    std::string      name_;
    ViewNodes        children_;
    ViewNode*        parent_;
    ViewNode*        activeChild_;
    ftxui::Component tab_;
    ftxui::Component childrenTabs_;
};

struct LineWithNumber
{
    std::string line;
    std::string lineNumber;
};

using LinesRingBuffer = utils::RingBuffer<LineWithNumber>;

struct View final : ViewNode
{
    View(std::string name, core::ViewId viewDataId);
    static ViewPtr create(std::string name, core::ViewId viewDataId);

    bool            loaded;
    core::ViewId    dataId;
    size_t          lineCount;
    size_t          viewHeight;
    size_t          lineNrDigits;
    size_t          yoffset;
    size_t          xoffset;
    size_t          ycurrent;
    bool            selectionMode;
    size_t          selectionPivot;
    size_t          selectionStart;
    size_t          selectionEnd;
    LinesRingBuffer ringBuffer;
};

void reloadLines(View& view, core::Context& context);
void reloadView(View& view, Ftxui& ui, core::Context& context);
LineWithNumber getLine(View& view, size_t lineIndex, core::Context& context);

}  // namespace ui

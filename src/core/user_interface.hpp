#pragma once

#include <memory>

#include "core/fwd.hpp"
#include "core/mode.hpp"
#include "core/views.hpp"
#include "utils/immobile.hpp"

namespace core
{

enum class Scroll : long
{
    beginning = 0,
    end       = -1,
};

enum class Parent
{
    root,
    currentView
};

using OpaqueWeakPtr = std::weak_ptr<void>;

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;
    virtual void run(Context& context) = 0;
    virtual void quit(Context& context) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual void scrollTo(Scroll lineNumber, Context& context) = 0;
    virtual OpaqueWeakPtr createView(std::string name, ViewId viewDataId, Parent parent, Context& context) = 0;
    virtual void removeView(OpaqueWeakPtr view, Context& context) = 0;
    virtual void onViewDataLoaded(OpaqueWeakPtr view, Context& context) = 0;
    virtual ViewId getCurrentView() = 0;
    virtual void onModeSwitch(Mode, Context&) = 0;

    template <typename T>
    T& get()
    {
        return static_cast<T&>(*this);
    }
};

}  // namespace core

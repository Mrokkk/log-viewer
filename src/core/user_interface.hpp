#pragma once

#include <iosfwd>

#include "core/fwd.hpp"
#include "core/mode.hpp"
#include "core/severity.hpp"
#include "utils/immobile.hpp"

namespace core
{

enum class Scroll : long
{
    beginning = 0,
    end       = -1,
};

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;
    virtual void run(Context& context) = 0;
    virtual void quit(Context& context) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual void scrollTo(Scroll lineNumber, Context& context) = 0;
    virtual std::ostream& operator<<(Severity severity) = 0;
    virtual void* createView(std::string name, Context& context) = 0;
    virtual void attachFileToView(File& file, void* view, Context& context) = 0;
    virtual void onModeSwitch(Mode, Context&) = 0;

    template <typename T>
    T& get()
    {
        return static_cast<T&>(*this);
    }
};

}  // namespace core

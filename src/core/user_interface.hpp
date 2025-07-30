#pragma once

#include <functional>
#include <iosfwd>
#include <memory>

#include "core/fwd.hpp"
#include "core/logger.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct TerminalSize
{
    int x;
    int y;
};

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;
    virtual TerminalSize getTerminalSize() const = 0;
    virtual void run(core::Context& context) = 0;
    virtual void quit(core::Context& context) = 0;
    virtual void execute(std::function<void()> fn) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual std::ostream& operator<<(Severity severity) = 0;

    template <typename T>
    T& get()
    {
        return static_cast<T&>(*this);
    }
};

using UserInterfacePtr = std::unique_ptr<UserInterface>;

}  // namespace core

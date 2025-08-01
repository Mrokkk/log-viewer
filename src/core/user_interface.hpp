#pragma once

#include <iosfwd>
#include <memory>

#include "core/fwd.hpp"
#include "core/logger.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct UserInterface : utils::Immobile
{
    virtual ~UserInterface() = default;
    virtual void run(Context& context) = 0;
    virtual void quit(Context& context) = 0;
    virtual void executeShell(const std::string& command) = 0;
    virtual void scrollTo(ssize_t lineNumber, Context& context) = 0;
    virtual std::ostream& operator<<(Severity severity) = 0;
    virtual void* createView(Context& context) = 0;
    virtual void attachFileToView(MappedFile& file, void* view, Context& context) = 0;

    template <typename T>
    T& get()
    {
        return static_cast<T&>(*this);
    }
};

using UserInterfacePtr = std::unique_ptr<UserInterface>;

}  // namespace core

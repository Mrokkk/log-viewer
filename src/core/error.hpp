#pragma once

#include <string>

#include "utils/fwd.hpp"

namespace core
{

struct Error final
{
    enum Type
    {
        Aborted = 1,
        SystemError,
        RegexError,
    };

    constexpr operator Type() const
    {
        return static_cast<Type>(mMessage.back());
    }

    constexpr bool operator==(Type error) const
    {
        return static_cast<Type>(mMessage.back()) == error;
    }

    constexpr std::string_view message() const
    {
        return std::string_view(mMessage.begin(), mMessage.end() - 1);
    }

    constexpr static Error aborted(std::string message)
    {
        return Error(Type::Aborted, std::move(message));
    }

    constexpr static Error systemError(std::string message)
    {
        return Error(Type::SystemError, std::move(message));
    }

    constexpr static Error regexError(std::string message)
    {
        return Error(Type::RegexError, std::move(message));
    }

private:
    constexpr Error(Type error, std::string message)
        : mMessage(message)
    {
        mMessage.insert(mMessage.end(), char(error));
    }

    std::string mMessage;
};

utils::Buffer& operator<<(utils::Buffer& buf, const Error& error);

}  // namespace core

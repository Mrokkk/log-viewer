#pragma once

#include <mutex>

#include "core/severity.hpp"
#include "utils/buffer.hpp"
#include "utils/string.hpp"

namespace core
{

struct MessageLine
{
    struct Flusher
    {
        Flusher(utils::Buffer& buffer, std::mutex& lock);
        ~Flusher();

        template <typename T>
        utils::Buffer& operator<<(T&& value)
        {
            return mBuffer << std::forward<T>(value);
        }

    private:
        utils::Buffer& mBuffer;
        std::mutex&    mLock;
    };

    MessageLine();
    ~MessageLine();

    constexpr Flusher info()
    {
        clear();
        mSeverity = Severity::info;
        return Flusher(mBuffer, mLock);
    }

    constexpr Flusher error()
    {
        clear();
        mSeverity = Severity::error;
        return Flusher(mBuffer, mLock);
    }

    void clear();
    Severity severity() const;
    std::string str() const;

    // FIXME: thread safety - client can iterate through it while
    // some thread might change content and lead to reallocation
    const utils::Strings& history() const;

private:
    std::mutex     mLock;
    Severity       mSeverity;
    utils::Buffer  mBuffer;
    utils::Strings mHistory;
};

}  // namespace core

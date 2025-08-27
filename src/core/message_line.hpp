#pragma once

#include <mutex>
#include <spanstream>

#include "core/severity.hpp"
#include "utils/string.hpp"

namespace core
{

struct MessageLine
{
    struct Flusher
    {
        Flusher(std::ostream& stream, std::mutex& lock);
        ~Flusher();

        template <typename T>
        std::ostream& operator<<(const T& value)
        {
            return stream_ << value;
        }

    private:
        std::ostream& stream_;
        std::mutex&   lock_;
    };

    MessageLine();
    ~MessageLine();

    Flusher operator<<(Severity severity);
    void clear();
    Severity severity() const;
    std::string get() const;

    // FIXME: thread safety - client can iterate through it while
    // some thread might change content and lead to reallocation
    const utils::Strings& history() const;

private:
    std::mutex        lock_;
    Severity          severity_;
    char              buffer_[256];
    std::ospanstream  stream_;
    utils::Strings    history_;
};

}  // namespace core

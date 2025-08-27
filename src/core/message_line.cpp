#include "message_line.hpp"

#include <cstring>
#include <mutex>
#include <ostream>

#include "core/severity.hpp"

namespace core
{

MessageLine::Flusher::Flusher(std::ostream& stream, std::mutex& lock)
    : stream_(stream)
    , lock_(lock)
{
}

MessageLine::Flusher::~Flusher()
{
    lock_.unlock();
}

MessageLine::MessageLine()
    : severity_(info)
    , buffer_{}
    , stream_(buffer_)
{
}

MessageLine::~MessageLine() = default;

MessageLine::Flusher MessageLine::operator<<(Severity severity)
{
    lock_.lock();

    clear();

    severity_ = severity;
    return Flusher(stream_, lock_);
}

void MessageLine::clear()
{
    if (buffer_[0] != '\0')
    {
        history_.emplace_back(stream_.span().data());
    }

    std::memset(buffer_, 0, sizeof(buffer_));
    std::ospanstream s(buffer_);
    stream_.swap(s);
}

Severity MessageLine::severity() const
{
    return severity_;
}

std::string MessageLine::get() const
{
    return stream_.span().data();
}

const utils::Strings& MessageLine::history() const
{
    return history_;
}

}  // namespace core

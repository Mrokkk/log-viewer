#include "message_line.hpp"

#include <mutex>

#include "core/severity.hpp"
#include "utils/buffer.hpp"

namespace core
{

MessageLine::Flusher::Flusher(utils::Buffer& buffer, std::mutex& lock)
    : mBuffer(buffer)
    , mLock(lock)
{
}

MessageLine::Flusher::~Flusher()
{
    mLock.unlock();
}

MessageLine::MessageLine()
    : mSeverity(Severity::info)
{
}

MessageLine::~MessageLine() = default;

void MessageLine::clear()
{
    if (mBuffer.length())
    {
        mHistory.emplace_back(mBuffer.str());
    }

    mBuffer.clear();
}

Severity MessageLine::severity() const
{
    return mSeverity;
}

std::string MessageLine::str() const
{
    return mBuffer.str();
}

const utils::Strings& MessageLine::history() const
{
    return mHistory;
}

}  // namespace core

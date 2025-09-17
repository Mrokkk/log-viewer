#include "logger.hpp"

#include <atomic>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string_view>

#include "core/severity.hpp"
#include "sys/log_printer.hpp"
#include "utils/buffer.hpp"
#include "utils/ring_buffer.hpp"
#include "utils/source_location.hpp"

using LogRingBuffer = utils::RingBuffer<LogEntry>;

namespace core
{

namespace
{

struct LoggerState
{
    std::atomic_bool closed{false};
    FILE*            fileStream{nullptr};
    std::mutex       lock;
    LogRingBuffer    ringBuffer{1024};
};

}  // namespace

static LoggerState state;

static thread_local utils::Buffer buffer;

Logger::Flusher Logger::log(Severity severity, LogEntryFlags flags, const char* header, utils::SourceLocation loc)
{
    return Flusher(severity, flags, header, loc, buffer);
}

void Logger::flushToStderr()
{
    state.closed = true;
    if (not state.fileStream)
    {
        state.ringBuffer.forEach(
            [](const LogEntry& entry)
            {
                sys::printLogEntry(entry, stderr);
            });
        fflush(stderr);
    }
}

void Logger::setLogFile(std::string_view path)
{
    std::string str(path);
    state.fileStream = fopen(str.c_str(), "w");

    if (not state.fileStream)
    {
        fprintf(stderr, "Failed to open: %s\n", str.c_str());
    }
}

LogEntries Logger::logEntries()
{
    LogEntries logs;
    logs.reserve(state.ringBuffer.size());
    state.ringBuffer.forEach(
        [&logs](const auto& entry)
        {
            logs.push_back(entry);
        });
    return logs;
}

void Logger::registerLogEntry(Severity severity, LogEntryFlags flags, const char* header, utils::SourceLocation loc)
{
    LogEntry entry{
        .severity = severity,
        .flags = flags,
        .time = std::time(nullptr),
        .location = loc,
        .header = header,
        .message = buffer.str(),
    };

    buffer.clear();

    std::scoped_lock scopedLock(state.lock);

    if (state.fileStream)
    {
        sys::printLogEntry(entry, state.fileStream);
        fflush(state.fileStream);
        return;
    }

    if (state.closed) [[unlikely]]
    {
        sys::printLogEntry(entry, stderr);
        fflush(stderr);
        return;
    }

    state.ringBuffer.pushBack(std::move(entry));
}

Logger::Flusher::Flusher(
    Severity severity,
    LogEntryFlags flags,
    const char* header,
    utils::SourceLocation loc,
    utils::Buffer& buffer)
    : mSeverity(severity)
    , mFlags(flags)
    , mHeader(header)
    , mLocation(loc)
    , mBuffer(buffer)
{
}

}  // namespace core

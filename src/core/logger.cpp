#include "logger.hpp"

#include <atomic>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string_view>

#include "core/severity.hpp"
#include "sys/log_printer.hpp"
#include "utils/buffer.hpp"
#include "utils/ring_buffer.hpp"
#include "utils/source_location.hpp"

using LogRingBuffer = utils::RingBuffer<LogEntry>;

static std::atomic_bool closed;
static std::ofstream    fileStream;
static std::mutex       lock;
static LogRingBuffer    ringBuffer(1024);

static thread_local utils::Buffer buffer;

Logger::Flusher Logger::log(Severity severity, LogEntryFlags flags, const char* header, utils::SourceLocation loc)
{
    return Flusher(severity, flags, header, loc, buffer);
}

void Logger::flushToStderr()
{
    closed = true;
    if (not fileStream.is_open())
    {
        ringBuffer.forEach(
            [](const LogEntry& entry)
            {
                sys::printLogEntry(entry, std::cerr);
            });
        std::cerr.flush();
    }
}

void Logger::setLogFile(std::string_view path)
{
    fileStream.open(path.data());
}

LogEntries Logger::logEntries()
{
    LogEntries logs;
    logs.reserve(ringBuffer.size());
    ringBuffer.forEach(
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

    std::scoped_lock scopedLock(lock);

    if (fileStream.is_open())
    {
        sys::printLogEntry(entry, fileStream);
        fileStream.flush();
    }

    if (closed) [[unlikely]]
    {
        sys::printLogEntry(entry, std::cerr);
        std::cerr.flush();
        return;
    }

    ringBuffer.pushBack(std::move(entry));
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

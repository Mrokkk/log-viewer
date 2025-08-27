#include "logger.hpp"

#include <atomic>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>

#include "utils/ring_buffer.hpp"

using StringsRingBuffer = utils::RingBuffer<std::string>;

const Logger             logger;
static std::atomic_bool  closed;
static std::ofstream     fileStream;
static std::mutex        lock;
static StringsRingBuffer ringBuffer(512);

Flusher Logger::operator<<(Severity severity) const
{
    return Flusher(severity);
}

void Logger::flushToStderr()
{
    closed = true;
    if (not fileStream.is_open())
    {
        ringBuffer.forEach(
            [](const auto& line)
            {
                std::cerr << line << '\n';
            });
    }
}

void Logger::setLogFile(std::string_view path)
{
    fileStream.open(path.data());
}

static void pushLine(std::string line)
{
    std::scoped_lock scopedLock(lock);

    if (fileStream.is_open())
    {
        fileStream << line << std::endl;
    }

    if (closed) [[unlikely]]
    {
        std::cerr << line << std::endl;
        return;
    }

    ringBuffer.pushBack(std::move(line));
}

Flusher::Flusher(Severity severity)
{
    std::time_t t = std::time(nullptr);
    line << "\e[32m[" << std::put_time(std::localtime(&t), "%F %T") << "]\e[0m ";

    switch (severity)
    {
        case warning:
            line << "\e[33m";
            break;
        case error:
            line << "\e[31m";
            break;
        default:
            break;
    }
}

Flusher::~Flusher()
{
    line << "\e[0m";
    pushLine(line.str());
}

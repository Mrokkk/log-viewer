#include "logger.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "utils/ring_buffer.hpp"

const Logger logger;
utils::RingBuffer<std::string> Logger::ringBuffer_(256);

std::ofstream fileStream;

Flusher Logger::operator<<(Severity severity) const
{
    return Flusher(severity);
}

void Logger::flushToStderr()
{
    if (not fileStream.is_open())
    {
        ringBuffer_.forEach(
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

void Logger::pushLine(std::string line)
{
    if (fileStream.is_open())
    {
        fileStream << line << std::endl;
    }
    ringBuffer_.pushBack(std::move(line));
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
    Logger::pushLine(line.str());
}

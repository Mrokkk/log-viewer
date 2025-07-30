#pragma once

#include <sstream>
#include <string>

#include "utils/immobile.hpp"
#include "utils/ring_buffer.hpp"

enum Severity
{
    debug,
    info,
    warning,
    error,
};

struct Flusher final : utils::Immobile
{
    Flusher(Severity severity);
    ~Flusher();

    template <typename T>
    Flusher& operator<<(const T& value)
    {
        line << value;
        return *this;
    }

private:
    std::ostringstream line;
};

struct Logger final : utils::Immobile
{
    Flusher operator<<(Severity severity) const;
    static void flushToStderr();
    static void setLogFile(std::string_view path);

private:
    friend Flusher;
    static void pushLine(std::string line);

    static utils::RingBuffer<std::string> ringBuffer_;
};

extern const Logger logger;

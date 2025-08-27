#pragma once

#include <sstream>

#include "core/severity.hpp"
#include "utils/immobile.hpp"

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
};

extern const Logger logger;

#pragma once

#include <ctime>
#include <string>

#include "core/severity.hpp"
#include "utils/bitflag.hpp"
#include "utils/source_location.hpp"

DEFINE_BITFLAG(LogEntryFlags, char,
{
    noSourceLocation,
});

struct LogEntry final
{
    using Flags = LogEntryFlags;

    Severity              severity;
    Flags                 flags;
    std::time_t           time;
    utils::SourceLocation location;
    const char*           header;
    std::string           message;
};

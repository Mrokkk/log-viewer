#pragma once

#include "core/log_entry.hpp"

#include <iosfwd>

namespace sys
{

void printLogEntry(const LogEntry& entry, std::ostream& os);

}  // namespace sys

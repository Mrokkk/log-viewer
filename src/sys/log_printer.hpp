#pragma once

#include <cstdio>

#include "core/log_entry.hpp"

namespace sys
{

void printLogEntry(const LogEntry& entry, FILE* file);

}  // namespace sys

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/file.hpp"
#include "core/mapping.hpp"

namespace sys
{

using Paths = std::vector<std::filesystem::path>;

void initialize();
void finalize();
void crashHandle(const int signal);
core::File fileOpen(std::string path);
void fileClose(core::File& file);
int remap(const core::File& file, core::Mapping& mapping, size_t newOffset, size_t newLen);
void unmap(core::Mapping& mapping);
void stacktraceLog();
Paths getConfigFiles();

}  // namespace sys

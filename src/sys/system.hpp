#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "sys/file.hpp"
#include "sys/mapping.hpp"

namespace sys
{

using Paths = std::vector<std::filesystem::path>;

void initialize();
void finalize();
void crashHandle(const int signal);
File fileOpen(std::string path);
void fileClose(File& file);
int remap(const File& file, Mapping& mapping, size_t newOffset, size_t newLen);
void unmap(Mapping& mapping);
void stacktraceLog();
Paths getConfigFiles();
int copyToClipboard(std::string string);

}  // namespace sys

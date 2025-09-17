#pragma once

#include <string>
#include <vector>

#include "sys/common.hpp"
#include "sys/file.hpp"
#include "sys/mapping.hpp"

namespace sys
{

using Paths = std::vector<std::string>;

const char* errorDescribe(Error error);
void initialize();
void finalize();
void crashHandle(const int signal);
MaybeFile fileOpen(std::string path);
Error fileClose(File& file);
Error remap(const File& file, Mapping& mapping, size_t newOffset, size_t newLen);
Error unmap(Mapping& mapping);
void stacktraceLog();
Paths getConfigFiles();
int copyToClipboard(std::string string);

}  // namespace sys

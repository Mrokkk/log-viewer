#include "assert.hpp"

#include <cstdlib>
#include <string_view>

#include "core/logger.hpp"

namespace core::detail
{

[[noreturn]] void assertionFailed(std::string_view message, const char* file, size_t line, const char* func)
{
    logger.error() << "Assertion failed: " << file << ':' << line << ":" << func << ": " << message;
    std::abort();
}

}  // namespace core::detail

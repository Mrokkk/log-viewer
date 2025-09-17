#include "buffer.hpp"

#include <string>

namespace utils
{

Buffer& Buffer::operator<<(const std::string& string)
{
    return operator<<(std::string_view(string));
}

std::string Buffer::str() const
{
    return std::string(mBuffer, mSize);
}

}  // namespace utils

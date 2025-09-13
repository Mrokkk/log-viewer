#include "error.hpp"

#include "utils/buffer.hpp"

namespace core
{

utils::Buffer& operator<<(utils::Buffer& buf, const Error& error)
{
    switch (error)
    {
        case Error::Aborted:
            buf << "[Aborted] ";
            break;

        case Error::SystemError:
            buf << "[System error] ";
            break;

        case Error::RegexError:
            buf << "[Regex error] ";
            break;
    }
    return buf << error.message();
}

}  // namespace core

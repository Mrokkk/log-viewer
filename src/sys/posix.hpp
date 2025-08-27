#pragma once

#ifdef __unix__

namespace sys
{

using Error = int;
using FileDescriptor = int;

}  // namespace sys

#endif  // __unix__

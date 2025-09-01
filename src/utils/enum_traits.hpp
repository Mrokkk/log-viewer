#pragma once

#include <type_traits>

namespace utils
{

template <typename T>
concept IsEnum = std::is_enum_v<T>;

}  // namespace utils

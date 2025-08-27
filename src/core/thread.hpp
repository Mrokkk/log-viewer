#pragma once

#include <functional>

namespace core
{

void async(std::function<void()> work);
bool isMainThread();

}  // namespace core

#pragma once

#include <functional>
#include <vector>

namespace core
{

using Task = std::function<void()>;
using Tasks = std::vector<Task>;

void async(Task task);
void executeInParallelAndWait(Tasks tasks);
bool isMainThread();
unsigned hardwareThreadCount();

}  // namespace core

#include "thread.hpp"

#include <thread>
#include <vector>

namespace core
{

using Thread = std::thread;
using Threads = std::vector<Thread>;

static const auto mainThreadId = std::this_thread::get_id();

void async(Task task)
{
    std::thread(std::move(task)).detach();
}

void executeInParallelAndWait(Tasks tasks)
{
    Threads threads;
    threads.reserve(tasks.size());
    for (auto& task : tasks)
    {
        threads.emplace_back(std::move(task));
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
}

bool isMainThread()
{
    return std::this_thread::get_id() == mainThreadId;
}

unsigned hardwareThreadCount()
{
    return std::thread::hardware_concurrency();
}

}  // namespace core

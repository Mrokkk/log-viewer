#include "thread.hpp"

#include <thread>

namespace core
{

const auto mainThreadId = std::this_thread::get_id();

void async(std::function<void()> work)
{
    std::thread(std::move(work)).detach();
}

void executeInParallelAndWait(Tasks tasks)
{
    std::vector<std::thread> threads;
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

#include "thread.hpp"

#include <thread>

namespace core
{

const auto mainThreadId = std::this_thread::get_id();

void async(std::function<void()> work)
{
    std::thread(std::move(work)).detach();
}

bool isMainThread()
{
    return std::this_thread::get_id() == mainThreadId;
}

}  // namespace core

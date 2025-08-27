#include "time.hpp"

#include <chrono>

#include "utils/immobile.hpp"
#include "utils/memory.hpp"

namespace utils
{

using SystemClock = std::chrono::system_clock;
using Time = decltype(SystemClock::now());

template <typename T>
using Duration = std::chrono::duration<T>;

struct TimerData : utils::Immobile
{
    constexpr TimerData(Time time)
        : start(time)
    {
    }

    Time start;
};

TimerData& getData(long& data)
{
    return *reinterpret_cast<TimerData*>(&data);
}

const TimerData& getData(const long& data)
{
    return *reinterpret_cast<const TimerData*>(&data);
}

Timer::Timer()
{
    static_assert(sizeof(Timer::data_) >= sizeof(TimerData));
    auto& data = getData(data_);
    constructAt(&data, SystemClock::now());
}

Timer::~Timer()
{
    destroyAt(&getData(data_));
}

float Timer::elapsed() const
{
    const auto start = getData(data_).start;
    const auto end = SystemClock::now();
    Duration<float> elapsed = end - start;
    return elapsed.count();
}

Timer startTimeMeasurement()
{
    return Timer();
}

}  // namespace utils

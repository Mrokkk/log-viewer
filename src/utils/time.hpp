#pragma once

namespace utils
{

struct Timer final
{
    Timer();
    ~Timer();
    float elapsed() const;

private:
    long mData;
};

Timer startTimeMeasurement();

}  // namespace utils

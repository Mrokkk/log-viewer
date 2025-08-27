#pragma once

namespace utils
{

struct Timer final
{
    Timer();
    ~Timer();
    float elapsed() const;

private:
    long data_;
};

Timer startTimeMeasurement();

}  // namespace utils

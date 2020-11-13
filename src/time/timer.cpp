#include <chrono> // Timing
#include <iomanip>

#include "mutils/logger/logger.h"
#include "mutils/time/timer.h"
#include <mutils/time/profiler.h>

using namespace std::chrono;

namespace MUtils
{

timer globalTimer;

uint64_t timer_get_time_now()
{
    return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

void timer_start(timer& timer)
{
    timer_restart(timer);
}

void timer_restart(timer& timer)
{
    timer.startTime = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

uint64_t timer_get_elapsed(const timer& timer)
{
    auto now = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    return now - timer.startTime;
}

double timer_get_elapsed_seconds(const timer& timer)
{
    return timer_to_seconds(timer_get_elapsed(timer));
}

double timer_to_seconds(uint64_t value)
{
    return double(value / 1000000000.0);
}

double timer_to_ms(uint64_t value)
{
    return double(value / 1000000.0);
}

} // namespace MUtils

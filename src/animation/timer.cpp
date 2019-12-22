#include <chrono> // Timing
#include <iomanip>

#include "mutils/logger/logger.h"
#include "mutils/animation/timer.h"

namespace MUtils
{

struct TimedSection
{
    uint64_t elapsed = 0;
    uint64_t count;
};

timer globalTimer;
profile_data globalProfiler;

uint64_t timer_get_time_now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void timer_start(timer& timer)
{
    timer_restart(timer);
}

void timer_restart(timer& timer)
{
    timer.startTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

uint64_t timer_get_elapsed(const timer& timer)
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return now - timer.startTime;
}

double timer_get_elapsed_seconds(const timer& timer)
{
    return timer_to_seconds(timer_get_elapsed(timer));
}

double timer_to_seconds(uint64_t value)
{
    return double(value / 1000000.0);
}

double timer_to_ms(uint64_t value)
{
    return double(value / 1000.0);
}

ProfileBlock::ProfileBlock(uint32_t timerId)
    : Id(timerId)
{
    timer_start(blockTimer);
}

ProfileBlock::~ProfileBlock()
{
    std::lock_guard<std::mutex>(globalProfiler.profile_mutex);
    elapsed = timer_get_elapsed(blockTimer);
    profile_add_value(globalProfiler.timerData[Id], double(elapsed));
}

void profile_add_value(profile_value& val, double av)
{
    std::lock_guard<std::mutex>(globalProfiler.profile_mutex);
    val.count++;
    val.average = val.average * (val.count - 1) / val.count + av / val.count;
    val.current = av;
    val.total += av;
}

uint32_t profile_get_id(const char* strName)
{
    std::lock_guard<std::mutex>(globalProfiler.profile_mutex);

    if (globalProfiler.currentId == 0)
    {
        timer_start(globalProfiler.globalTimer);
    }
    globalProfiler.nameToId[strName] = globalProfiler.currentId;
    globalProfiler.IdToName[globalProfiler.currentId] = strName;
    if (globalProfiler.timerData.size() <= (globalProfiler.currentId + 1))
    {
        globalProfiler.timerData.resize(globalProfiler.currentId + 1);
    }
    uint32_t val = globalProfiler.currentId;
    globalProfiler.currentId++;

    return val;
}

} // namespace MUtils

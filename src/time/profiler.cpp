#include <chrono> // Timing
#include <iomanip>

#include "mutils/logger/logger.h"
#include "mutils/time/timer.h"
#include <mutils/time/profiler.h>

namespace MUtils
{

struct TimedSection
{
    uint64_t elapsed = 0;
    uint64_t count;
};

profile_data globalProfiler;

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

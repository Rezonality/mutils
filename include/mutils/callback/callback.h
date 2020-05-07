#pragma once

#include <functional>
#include "mutils/time/timer.h"

namespace MUtils
{

enum class SchedulerState
{
    Off = 0,
    Started = 1,
    Triggered = 2
};

struct scheduler
{
    std::function<void()> callback;
    timer cb_timer;
    float timeout = 1.0f;
    SchedulerState state = SchedulerState::Off;
};

void scheduler_start(scheduler& sched, float seconds, std::function<void()> fnCallback);
void scheduler_stop(scheduler& sched);
void scheduler_update(scheduler& sched);

} // MUtils

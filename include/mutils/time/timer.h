#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>

namespace MUtils
{

enum class TimerSample : uint32_t
{
    None,
    Restart
};

struct timer
{
    int64_t startTime = 0;
};
extern timer globalTimer;

uint64_t timer_get_time_now();
void timer_restart(timer& timer);
void timer_start(timer& timer);
uint64_t timer_get_elapsed(const timer& timer);
double timer_get_elapsed_seconds(const timer& timer);
double timer_to_seconds(uint64_t value);
double timer_to_ms(uint64_t value);

} // namespace MUtils

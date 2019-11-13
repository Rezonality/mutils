#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

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

struct profile_value
{
    double average = 0;
    double current = 0;
    double total = 0;
    uint64_t count = 0;
};

struct profile_data
{
    std::vector<profile_value> timerData;
    std::unordered_map<std::string, uint32_t> nameToId;
    std::unordered_map<uint32_t, std::string> IdToName;
    uint32_t currentId = 0;
    std::mutex profile_mutex;
};

extern profile_data globalProfiler;

void profile_add_value(profile_value& val, double av);
uint32_t profile_get_id(const char* strName);

class ProfileBlock
{
public:
    uint32_t Id;
    timer blockTimer;
    uint64_t elapsed = 0;

    ProfileBlock(uint32_t timerId);
    ~ProfileBlock();
};

#define TIMING
#ifdef TIMING
#define PROFILE_SCOPE(name)  \
    static const uint32_t name##_timer_id = MUtils::profile_get_id(#name); \
    MUtils::ProfileBlock name##_timer_block(name##_timer_id);

#else
#define PROFILE_SCOPE(name)
#endif

} // namespace MUtils

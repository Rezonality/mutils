#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>

namespace MUtils
{

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
    timer globalTimer;
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

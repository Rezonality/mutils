#pragma once

#include <thread>
#include <mutils/time/timer.h>
#include <mutils/math/math.h>
#include <mutils/ui/theme.h>

namespace MUtils
{

namespace Profiler
{

struct ProfilerEntry
{
    // static infos
    const char* szSection;
    const char* szFile;
    int line;
    unsigned int color;
    // infos used for rendering infos
    int level = 0;
    int64_t startTime;
    int64_t endTime;
    uint32_t parent;
};

struct FrameThreadInfo
{
    uint32_t threadIndex;
    uint32_t activeEntry;
};

struct Region
{
    std::string name;
    int64_t startTime;
    int64_t endTime;
};

struct Frame : Region
{
    uint32_t frameThreadCount = 0;
    std::vector<FrameThreadInfo> frameThreads;
};

struct ThreadData
{
    bool initialized;
    uint32_t callStackDepth = 0;
    uint32_t maxLevel = 0;
    int64_t minTime;
    int64_t maxTime;
    uint32_t currentEntry = 0;
    bool hidden = false;
    std::string name;
    std::vector<ProfilerEntry> entries;
    std::vector<uint32_t> entryStack;
};

void Init();
void NewFrame();
void NameThread(const char* pszName);
void SetPaused(bool pause);
void BeginRegion();
void EndRegion();
void SetRegionLimit(uint64_t maxTimeNs);
void PushSectionBase(const char*, uint32_t, const char*, int);
void PopSection();
void ShowProfile(bool* opened);
void HideThread();
void Finish();

struct ProfileScope
{
    ProfileScope(const char* szSection, uint32_t color, const char* szFile, int line)
    {
        PushSectionBase(szSection, color, szFile, line);
    }
    ~ProfileScope()
    {
        PopSection();
    }
};

struct RegionScope
{
    RegionScope()
    {
        BeginRegion();
    }
    ~RegionScope()
    {
        EndRegion();
    }
};
#define PROFILE_COL_LOCK 0xFF0000FF

template <class _Mutex>
class profile_lock_guard { // class with destructor that unlocks a mutex
public:
    using mutex_type = _Mutex;

    explicit profile_lock_guard(_Mutex& _Mtx, const char* name = "Mutex", const char* szFile = nullptr, int line = 0) : _MyMutex(_Mtx) { // construct and lock
        PushSectionBase(name, PROFILE_COL_LOCK, szFile, line);
        _MyMutex.lock();
        PopSection();
    }

    profile_lock_guard(_Mutex& _Mtx, std::adopt_lock_t) : _MyMutex(_Mtx) {} // construct but don't lock

    ~profile_lock_guard() noexcept {
        _MyMutex.unlock();
    }

    profile_lock_guard(const profile_lock_guard&) = delete;
    profile_lock_guard& operator=(const profile_lock_guard&) = delete;

private:
    _Mutex& _MyMutex;
};

#define LOCK_GUARD(var, name) \
::MUtils::Profiler::profile_lock_guard name##_lock(var, #name, __FILE__, __LINE__)


} // namespace Profiler
} // namespace MUtils

// PROFILE_SCOPE(MyNameWithoutQuotes)
#define PROFILE_SCOPE(name) \
static const uint32_t name##_color = ToPackedARGB(MUtils::Theme::ThemeManager::ColorFromName(#name, sizeof(#name))); \
MUtils::Profiler::ProfileScope name##_scope(#name, name##_color, __FILE__, __LINE__);

// PROFILE_SCOPE(char*, ImColor32 bit value)
#define PROFILE_SCOPE_STR(str, col) \
MUtils::Profiler::ProfileScope name##_scope(str, col, __FILE__, __LINE__);

// Mark one extra region
#define PROFILE_REGION(name) \
MUtils::Profiler::RegionScope name##_region;

// Give a thread a name.
#define PROFILE_NAME_THREAD(name) \
MUtils::Profiler::NameThread(#name);

// Hide a thread.  Not sure this is tested or works....
#define PROFILE_HIDE_THREAD() \
MUtils::Profiler::HideThread();




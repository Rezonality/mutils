#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace MUtils
{

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
struct TimeEvent
{
    TimePoint time;
    uint64_t tickCount;
    uint32_t beat;
};

struct ITimeConsumer
{
    virtual void AddTimeEvent(const TimeEvent& ev) = 0;
};

class TimeProvider
{
public:
    TimeProvider();
    static TimeProvider& Instance();

    TimePoint Now() const;

    TimePoint StartTime() const;

    void ResetStartTime();
    void RegisterConsumer(ITimeConsumer* pConsumer);
    void UnRegisterConsumer(ITimeConsumer* pConsumer);
    void StartThread();
    void EndThread();

    void SetTickRate(uint32_t bpm, uint32_t beatsPerBar);
    void SetTickCount(uint32_t beat);

    uint32_t GetBeat() const;
    uint32_t GetBeatsPerMinute() const;
    uint32_t GetTickCount() const;

    void Tick();
private:
    TimePoint m_startTime;
    std::unordered_set<ITimeConsumer*> m_consumers;
    std::mutex m_mutex;

    std::atomic_bool m_quitTimer = false;
    std::thread m_tickThread;
    std::atomic<uint32_t> m_beatsPerBar = 4;
    std::atomic<uint32_t> m_beatsPerMinute = 120;
    std::atomic<float> m_timePerBeat = (1000.0f / 120.0f);
    std::atomic<uint32_t> m_tickCount = 0;
};

}; // namespace MUtils

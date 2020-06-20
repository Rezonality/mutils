#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <mutils/logger/logger.h>
#include <mutils/profile/profile.h>
#include <mutils/thread/mempool.h>

#include <concurrentqueue/concurrentqueue.h>

#include <ctti/type_id.hpp>

#undef ERROR
namespace MUtils
{

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

static const uint32_t TimeLineStorageSpace = 4;

inline float ToFloatSeconds(const TimePoint& pt)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(pt.time_since_epoch()).count() / 1000.0f;
}

// TODO: Seperate events from the timeline; time line should be a global/clean time ticker.
// Events should be seperate I think
class TimeLineEvent : public MUtils::PoolItem
{
public:
    DECLARE_POOL_ITEM(TimeLineEvent);

    TimeLineEvent(IMemoryPool* pPool, uint64_t id)
        : PoolItem(pPool, id)
    {
    }

    void SetTime(TimePoint t, std::chrono::milliseconds d = std::chrono::milliseconds(0))
    {
        m_time = t;
        m_duration = d;
    }

    void SetName(const char* pszName)
    {
        m_pszName = pszName;
    }

    TimePoint EndTime() const
    {
        return m_time + m_duration;
    }

    virtual void Init() override
    {
        // TODO: Cludge for now; used by note display: remove it
        for (uint32_t i = 0; i < TimeLineStorageSpace; i++)
        {
            m_storage[i] = (uint32_t)-1;
        }
        m_triggered = false;
        m_pNext = nullptr;
        m_pPrevious = nullptr;
    }

    TimePoint m_time;
    std::chrono::milliseconds m_duration;
    bool m_triggered = false;

    const char* m_pszName = nullptr;

    // Spare
    uint32_t m_storage[TimeLineStorageSpace];
};

bool timeline_validate(TimeLineEvent* pEvent);
void timeline_dump(TimeLineEvent* pRoot, TimeLineEvent* pCheckAbsent = nullptr);

// TODO: Extract note event from timeline; they are different concepts?
class NoteEvent : public MUtils::TimeLineEvent
{
public:
    DECLARE_POOL_ITEM(NoteEvent);

    NoteEvent(MUtils::IMemoryPool* pPool, uint64_t id)
        : MUtils::TimeLineEvent(pPool, id)
    {
    }

    float velocity = 1.0f;      // Velocity note was hit with; also like amplitude
    uint32_t midiNote = 0;      // Midi note
    int64_t instrumentId = -1;  // Unique Id, -1 for all
    int32_t channelId = -1;     // Assigned channel, or just use Midi
    float frequency = 0.0f;     // Frequency; if 0, then use midi
    bool pressed = false;
    bool transition = true;     // Transitioned to press or release
    bool inactive = false;      // Has this note gone inactive due to finishing being played?
};

struct ITimeConsumer
{
    virtual void AddTickEvent(TimeLineEvent* ev) = 0;
};

class TimeProvider
{
public:
    TimeProvider();
    static TimeProvider& Instance();

    void Free();

    TimePoint Now() const;
    TimePoint StartTime() const;

    void ResetStartTime();
    void RegisterConsumer(ITimeConsumer* pConsumer);
    void UnRegisterConsumer(ITimeConsumer* pConsumer);
    void StartThread();
    void EndThread();

    void SetTempo(double bpm, double quantum);
    void SetBeat(double beat);
    void SetFrame(uint32_t frame);

    uint32_t GetFrame() const;
    double GetBeat() const;
    double GetTempo() const;
    double GetQuantum() const;
    std::chrono::microseconds GetTimePerBeat() const;

    double GetBeatAtTime(TimePoint time);

    void Beat();

    void StoreTimeEvent(TimeLineEvent* event);
    void GetTimeEvents(std::vector<TimeLineEvent*>& events);
    void DequeTimeEvents(std::vector<TimeLineEvent*>& events, ctti::type_id_t type, TimePoint upTo);

    TSMemoryPool<TimeLineEvent>& GetEventPool()
    {
        return m_timeEventPool;
    };

    void InsertBefore(IListItem* pPos, IListItem* pItem);
    bool Validate() const;
    void Dump();

private:
    TimePoint m_startTime;
    std::unordered_set<ITimeConsumer*> m_consumers;
    MUtilsLockable(std::recursive_mutex, m_mutex);

    std::atomic_bool m_quitTimer = false;
    std::thread m_tickThread;
    std::atomic<double> m_quantum = 4;
    std::atomic<double> m_tempo = 120;
    std::atomic<double> m_beat = 0;
    std::atomic<uint32_t> m_frame = 0;

    TimePoint m_lastTime; // Should probably be atomic, but compile error on GCC?
    std::atomic<double> m_lastBeat;

    TSMemoryPool<TimeLineEvent> m_timeEventPool;

    std::chrono::microseconds m_timePerBeat = std::chrono::microseconds(1000000 / 120);
};

}; // namespace MUtils

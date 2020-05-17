#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_set>

#include <mutils/profile/profile.h>

#include <concurrentqueue/concurrentqueue.h>

namespace MUtils
{

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

enum TimeEventKinds
{
    TimeLine = 0,
    Orca = 1,
    SP = 2,
    JPat = 3,
    Ixi = 4
};

struct IEventPool
{
    virtual void Free(void* pEv) = 0;
};

static const uint32_t TimeLineStorageSpace = 4;
struct TimeLineEvent
{
    TimeLineEvent(IEventPool* pPool, uint32_t type, uint64_t id)
        : m_pPool(pPool)
        , m_type(type)
        , m_id(id)
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

    virtual void Free()
    {
        m_pPool->Free(this);
    }

    IEventPool* m_pPool;
    uint32_t m_type;
    uint64_t m_id;
    TimePoint m_time;
    std::chrono::milliseconds m_duration;
    bool m_triggered = false;
    const char* m_pszName;

    // Spare
    uint32_t m_storage[TimeLineStorageSpace];
};

struct NoteEvent : MUtils::TimeLineEvent
{
    NoteEvent(MUtils::IEventPool* pPool, uint32_t type, uint64_t id)
        : MUtils::TimeLineEvent(pPool, type, id)
    {
    }

    float velocity;
    uint32_t midiNote;
};

template <class T>
class EventPool : public IEventPool
{
public:
    EventPool(uint32_t type)
        : m_type(type)
    {
    }

    ~EventPool()
    {
        Clear();
    }

    void Clear()
    {
        T* pVictim = nullptr;
        while (m_freeItems.try_dequeue(pVictim))
        {
            delete pVictim;
        }
    }

    T* Alloc()
    {
        T* pRet = nullptr;

        if (!m_freeItems.try_dequeue(pRet))
        {
            pRet = new T(this, m_type, m_nextId++);
        }
        else
        {
            pRet->m_id = m_nextId++;

        }

        pRet->m_triggered = false;

        // TODO: Cludge for now; used by note display: remove it
        for (uint32_t i = 0; i < TimeLineStorageSpace; i++)
        {
            pRet->m_storage[i] = (uint32_t)-1;
 
        }
        return pRet;
    }

    void Free(void* pVal)
    {
        // store the free item for later
        auto pTyped = (T*)pVal;
        pTyped->m_id = (uint64_t)-1;
        assert(pTyped->m_type == m_type);
        m_freeItems.enqueue(pTyped);
    }

private:
    moodycamel::ConcurrentQueue<T*> m_freeItems;
    uint32_t m_type;
    uint64_t m_nextId = 0;
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

    void Beat();

    void StoreTimeEvent(TimeLineEvent* event);
    void GetTimeEvents(std::vector<TimeLineEvent*>& events);
    void DequeTimeEvents(std::vector<TimeLineEvent*>& events, uint32_t type, TimePoint upTo);

    EventPool<TimeLineEvent>& GetEventPool() { return m_timeEventPool; };

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

    EventPool<TimeLineEvent> m_timeEventPool;
    std::map<TimePoint, TimeLineEvent*> m_timeEvents;

    std::chrono::microseconds m_timePerBeat = std::chrono::microseconds(1000000 / 120);
};

}; // namespace MUtils

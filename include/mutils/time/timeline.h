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
#include <mutils/time/time_utils.h>
#include <mutils/time/time_provider.h>
#include <mutils/string/stringutils.h>

#include <concurrentqueue/concurrentqueue.h>

#include <ctti/type_id.hpp>

#undef ERROR
namespace MUtils
{

static const uint32_t TimeLineStorageSpace = 4;

// An event in the time line, stored in a memory pool
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

struct INoteOwner
{
}

class NoteEvent : public MUtils::TimeLineEvent
{
public:
    DECLARE_POOL_ITEM(NoteEvent);

    NoteEvent(MUtils::IMemoryPool* pPool, uint64_t id)
        : MUtils::TimeLineEvent(pPool, id)
    {
    }

    float velocity = 1.0f; // Velocity note was hit with; also like amplitude
    uint32_t midiNote = 0; // Midi note
    int64_t instrumentId = -1; // Unique Id, -1 for all
    int32_t channelId = -1; // Assigned channel, or just use Midi
    float frequency = 0.0f; // Frequency; if 0, then use midi
    bool pressed = false;
    bool transition = true; // Transitioned to press or release
    bool inactive = false; // Has this note gone inactive due to finishing being played?
    float activeAmplitude = 0.0f; // Amplitude of active note
    uint64_t groupId = 0;

    INoteOwner* pOwner = nullptr; // An owner for this note; perhaps a pattern processor or something else
    uint32_t ownerOffset = 0;     // Offset inside the owner
    uint32_t ownerSize = 0;       // Size inside the owner
};

template <class T>
class Timeline
{
public:
    Timeline()
        : m_timeEventPool(1000)
    {
        m_startTime = TimeProvider::Instance().Now();
    }


    void Free()
    {
        // Clear the pool
        m_timeEventPool.Clear();
    }

    TimePoint StartTime() const
    {
        return m_startTime;
    }

    void ResetStartTime()
    {
        m_startTime = TimeProvider::Instance().Now();
    }

    void ExpireEvents(int secondsOld)
    {
        std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);

        auto startTime = TimeProvider::Instance().Now();

        auto pCurrent = (T*)m_timeEventPool.m_pRoot;
        while (pCurrent)
        {
            // TODO: this is broken if the duration is long!
            // Need to track and remove long events
            if ((startTime - (pCurrent->m_time + pCurrent->m_duration)) > std::chrono::seconds(secondsOld))
            {
                auto pVictim = pCurrent;
                pCurrent = (T*)list_disconnect((IListItem*)pCurrent);
                pVictim->Free();
                continue;
            }
            else if ((startTime - pCurrent->m_time) < std::chrono::seconds(16))
            {
                break;
            }
            pCurrent = (T*)pCurrent->m_pNext;
        }
    }

    void StoreTimeEvent(T* ev)
    {
        std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
        assert(ev->m_pNext == nullptr);
        assert(ev->m_pPrevious == nullptr);

        // Add on the end, but keep events ordered in time
        auto pCurrent = (T*)m_timeEventPool.m_pLast;
        while (pCurrent && (ev->m_time < pCurrent->m_time))
        {
            pCurrent = (T*)pCurrent->m_pPrevious;
        }
        assert(pCurrent == nullptr || (pCurrent->m_time <= ev->m_time));
        list_insert_after(pCurrent, ev);
    }

    // TODO: Don't think this is necessary any more; since time events have linked lists
    void GetTimeEvents(std::vector<T*>& ev)
    {
        std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
        ev.clear();

        auto pCurrent = m_timeEventPool.m_pRoot;
        while (pCurrent)
        {
            ev.push_back((T*)pCurrent);
            pCurrent = pCurrent->m_pNext;
        }
    }

    // Returns events before the time, in an array
    // TODO: Just return links to begin/end?
    void DequeTimeEvents(std::vector<T*>& ev, ctti::type_id_t type, TimePoint upTo)
    {
        std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
        ev.clear();

        //LOG(DEBUG) << "T: " << time_to_float_seconds(upTo);

        TimePoint last = TimePoint::min();
        auto pCurrent = (T*)m_timeEventPool.m_pRoot;
        while (pCurrent)
        {
            //LOG(DEBUG) << "E: " << pCurrent->m_time.time_since_epoch().count();
            if (pCurrent->m_triggered)
            {
                pCurrent = (T*)pCurrent->m_pNext;
                continue;
            }

            if (pCurrent->m_time > upTo)
            {
                break;
            }

            if (pCurrent->GetType() == type)
            {
                pCurrent->m_triggered = true;
                ev.push_back(pCurrent);
                assert(pCurrent->m_time >= last);
                last = pCurrent->m_time;
                //event_dump("DQ: ", pCurrent);
            }
            pCurrent = (T*)pCurrent->m_pNext;
        }
    }

    // Getters
    TSMemoryPool<T>& GetEventPool()
    {
        return m_timeEventPool;
    };

private:
    TimePoint m_startTime;
    TSMemoryPool<T> m_timeEventPool;

    MUtilsLockable(std::recursive_mutex, m_mutex);
};

}; // namespace MUtils


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
#include <mutils/string/string_utils.h>
#include <mutils/thread/mempool.h>
#include <mutils/thread/thread_utils.h>
#include <mutils/time/time_provider.h>
#include <mutils/time/time_utils.h>

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
        m_triggered = false;
    }

    TimePoint m_time;
    std::chrono::milliseconds m_duration;
    bool m_triggered = false;
    const char* m_pszName = nullptr;
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
        PROFILE_SCOPE(ExpireEvents);
        LOCK_GUARD(m_mutex, Timeline_Lock);

        auto startTime = TimeProvider::Instance().Now();

        auto pCurrent = (T*)m_timeEventPool.m_pRoot;
        while (pCurrent)
        {
            // TODO: this is broken if the duration is long!
            // Need to track and remove long events
            if ((startTime - (pCurrent->m_time + pCurrent->m_duration)) > std::chrono::seconds(secondsOld))
            {
                auto pVictim = pCurrent;
                pCurrent = (T*)list_disconnect(gsl::not_null<IListItem*>(pCurrent));
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
        LOCK_GUARD(m_mutex, Timeline_Lock);
        assert(ev->m_pNext == nullptr);
        assert(ev->m_pPrevious == nullptr);

        // Add on the end, but keep events ordered in time
        auto pCurrent = (T*)m_timeEventPool.m_pLast;
        while (pCurrent && (ev->m_time < pCurrent->m_time))
        {
            pCurrent = (T*)pCurrent->m_pPrevious;
        }
        assert(pCurrent == nullptr || (pCurrent->m_time <= ev->m_time));
        list_insert_after(pCurrent, gsl::not_null<IListItem*>(ev));
    }

    // TODO: Don't think this is necessary any more; since time events have linked lists
    void GetTimeEvents(std::vector<T*>& ev)
    {
        PROFILE_SCOPE(GetTimeEvents);
        LOCK_GUARD(m_mutex, Timeline_Lock);
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
        PROFILE_SCOPE(DequeTimeEvents);
        LOCK_GUARD(m_mutex, Timeline_Lock);
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

    audio_spin_mutex m_mutex;
};

}; // namespace MUtils

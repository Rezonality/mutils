#include <algorithm>

#include "mutils/logger/logger.h"
#include "mutils/profile/profile.h"
#include "mutils/time/time_provider.h"

#include <ctti/type_id.hpp>

using namespace std::chrono;
namespace MUtils
{

bool timeline_validate(TimeLineEvent* pEvent)
{
    /*
    auto pRoot = list_root(pEvent);
    if (!pRoot)
        return true;

    auto pCurrent = pRoot;

    auto time = pCurrent->m_time;
    while (pCurrent->m_pNext)
    {
        if (pCurrent->m_pNext->m_time < time)
        {
            LOG(ERROR) << "Timeline not ordered!";
            timeline_dump(pRoot);
            assert(!"Timeline corrupt?");
            return false;
        }
        time = pCurrent->m_pNext->m_time;
        pCurrent = pCurrent->m_pNext;
    }
    */
    return true;
}

void event_dump(const char* pszType, TimeLineEvent* pRoot)
{
    LOG(DEBUG) << pszType << ToFloatSeconds(pRoot->m_time) << " ID: " << pRoot->m_id << " Trig: " << pRoot->m_triggered << " p:" << pRoot << " pN:" << pRoot->m_pNext << " pP:" << pRoot->m_pPrevious;
}

void timeline_dump(TimeLineEvent* pRoot, TimeLineEvent* pCheckAbsent)
{
    //TODO: Fix
    (void)&pRoot;
    (void)&pCheckAbsent;
    /*
    while (pRoot)
    {
        event_dump("Dump: ", pRoot);
        if (pCheckAbsent != nullptr && pRoot == pCheckAbsent)
        {
            LOG(ERROR) << "Already in timeline!";
            assert(!"Alread in timeline?");
        }
        pRoot = pRoot->m_pNext;
    }
    */
}

TimeProvider& TimeProvider::Instance()
{
    static TimeProvider provider;
    return provider;
}

TimeProvider::TimeProvider()
    : m_timeEventPool(1000)
{
    m_startTime = Now();
    SetTempo(120.0, 4.0);
}

void TimeProvider::Free()
{
    EndThread();

    // Clear the pool
    m_timeEventPool.Clear();
}

TimePoint TimeProvider::Now() const
{
    return std::chrono::high_resolution_clock::now();
}

TimePoint TimeProvider::StartTime() const
{
    return m_startTime;
}

void TimeProvider::ResetStartTime()
{
    m_startTime = Now();
}

void TimeProvider::RegisterConsumer(ITimeConsumer* pConsumer)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    m_consumers.insert(pConsumer);
}

void TimeProvider::UnRegisterConsumer(ITimeConsumer* pConsumer)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    m_consumers.erase(pConsumer);
}

void TimeProvider::Beat()
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);

    auto pEv = m_timeEventPool.Alloc();
    pEv->SetTime(TimeProvider::Instance().Now());
    static_cast<TimeLineEvent*>(pEv)->m_triggered = true; // Makes deubgging easier; this time event is temporary anyway; we don't really need a beat event

    //StoreTimeEvent(pEv);

    auto beat = m_beat.load();
    auto frame = m_frame.load();
    for (auto& consumer : m_consumers)
    {
        consumer->AddTickEvent(pEv);
    }

    m_frame.store(frame + 1);
    m_beat.store(beat + 1);
}

void TimeProvider::StartThread()
{
    //    static Moving_Average<uint64_t, uint64_t, 100> av;

    MUtilsNameThread("Time Provider");
    auto lastTime = TimeProvider::Instance().Now();

    // A thread which wakes up and ticks
    m_quitTimer = false;
    m_tickThread = std::thread([&]() {
        for (;;)
        {
            MUtilsZoneScopedN("TimeProvider Beat");
            // Beat at regular intervals, no matter how long our operation takes
            auto startTime = TimeProvider::Instance().Now();
            auto nextTime = startTime + m_timePerBeat;
            auto beat = m_beat.load();
            auto frame = m_frame.load();

            {
                std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);

                m_lastTime = startTime;
                m_lastBeat = beat;

                // Remove time events older than 8 beats before now
                // TODO: Lifetime?  When to destroy these?

                auto pCurrent = (TimeLineEvent*)m_timeEventPool.m_pRoot;
                while (pCurrent)
                {
                    // TODO: this is broken if the duration is long!
                    // Need to track and remove long events
                    if ((startTime - (pCurrent->m_time + pCurrent->m_duration)) > seconds(16))
                    {
                        auto pVictim = pCurrent;
                        pCurrent = (TimeLineEvent*)list_disconnect((IListItem*)pCurrent);
                        pVictim->Free();
                        continue;
                    }
                    else if ((startTime - pCurrent->m_time) < seconds(16))
                    {
                        break;
                    }
                    pCurrent = (TimeLineEvent*)pCurrent->m_pNext;
                }

                auto pEv = m_timeEventPool.Alloc();
                static_cast<TimeLineEvent*>(pEv)->m_triggered = true; // Makes deubgging easier; this time event is temporary anyway; we don't really need a beat event
                pEv->SetTime(startTime);

                //StoreTimeEvent(pEv);
                {
                    for (auto& consumer : m_consumers)
                    {
                        consumer->AddTickEvent(pEv);
                    }
                }

                m_frame.store(frame + 1);
                m_beat.store(beat + 1);
            }

            if (m_quitTimer.load())
            {
                break;
            }

            std::this_thread::sleep_until(nextTime);
        }
    });
}

void TimeProvider::EndThread()
{
    m_quitTimer = true;
    if (m_tickThread.joinable())
    {
        m_tickThread.join();
    }
}

double TimeProvider::GetBeatAtTime(TimePoint time)
{
    auto d = duration_cast<microseconds>(time - m_lastTime).count();
    auto beat = m_lastBeat.load();

    auto beatsPerTime = (double)d / (double)duration_cast<microseconds>(GetTimePerBeat()).count();
    return beat + beatsPerTime;
}

double TimeProvider::GetBeat() const
{
    return m_beat;
}

double TimeProvider::GetTempo() const
{
    return m_tempo;
}

double TimeProvider::GetQuantum() const
{
    return m_quantum;
}

void TimeProvider::SetTempo(double tempo, double quantum)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    m_quantum = quantum;
    m_tempo = tempo;
    m_timePerBeat = microseconds((uint64_t)(60000000.0 / tempo));
}

void TimeProvider::SetBeat(double beat)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    m_beat = beat;
}

void TimeProvider::StoreTimeEvent(TimeLineEvent* ev)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    //event_dump("Store: ", ev);
    assert(ev->m_pNext == nullptr);
    assert(ev->m_pPrevious == nullptr);
    //timeline_dump(m_pRoot, ev);

    list_insert_after(m_timeEventPool.m_pLast, ev);

}

// TODO: Don't think this is necessary any more; since time events have linked lists
void TimeProvider::GetTimeEvents(std::vector<TimeLineEvent*>& ev)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    ev.clear();

    auto pCurrent = m_timeEventPool.m_pRoot;
    while (pCurrent)
    {
        ev.push_back((TimeLineEvent*)pCurrent);
        pCurrent = pCurrent->m_pNext;
    }
}

// Returns events before the time, in an array
// TODO: Just return links to begin/end?
void TimeProvider::DequeTimeEvents(std::vector<TimeLineEvent*>& ev, ctti::type_id_t type, TimePoint upTo)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    ev.clear();

    //LOG(DEBUG) << "T: " << ToFloatSeconds(upTo);

    TimePoint last = TimePoint::min();
    auto pCurrent = (TimeLineEvent*)m_timeEventPool.m_pRoot;
    while (pCurrent)
    {
        //LOG(DEBUG) << "E: " << pCurrent->m_time.time_since_epoch().count();
        if (pCurrent->m_triggered)
        {
            pCurrent = (TimeLineEvent*)pCurrent->m_pNext;
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
        pCurrent = (TimeLineEvent*)pCurrent->m_pNext;
    }
}

void TimeProvider::SetFrame(uint32_t frame)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    m_frame.store(frame);
}

uint32_t TimeProvider::GetFrame() const
{
    return m_frame.load();
}

std::chrono::microseconds TimeProvider::GetTimePerBeat() const
{
    return m_timePerBeat;
}

bool TimeProvider::Validate() const
{
    return timeline_validate((TimeLineEvent*)m_timeEventPool.m_pRoot);
}

void TimeProvider::Dump()
{
    timeline_dump((TimeLineEvent*)m_timeEventPool.m_pRoot);
}
    

} // namespace MUtils

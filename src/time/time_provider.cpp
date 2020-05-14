#include <algorithm>

#include "mutils/logger/logger.h"
#include "mutils/profile/profile.h"
#include "mutils/time/time_provider.h"

using namespace std::chrono;
namespace MUtils
{

TimeProvider& TimeProvider::Instance()
{
    static TimeProvider provider;
    return provider;
}

TimeProvider::TimeProvider()
    : m_timeEventPool(0)
{
    m_startTime = Now();
    SetTempo(120.0, 4.0);
}

void TimeProvider::Free()
{
    EndThread();

    // Free all outstanding time events
    for (auto& [t, pEv] : m_timeEvents)
    {
        pEv->Free();
    }
    m_timeEvents.clear();

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

    auto pEv = m_timeEventPool.Alloc(TimeProvider::Instance().Now());
    StoreTimeEvent(pEv);

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

                // Remove time events older than 8 beats before now
                // TODO: Lifetime?  When to destroy these?
                auto itrEv = m_timeEvents.begin();
                while (itrEv != m_timeEvents.end())
                {
                    if ((startTime - itrEv->first) > seconds(16))
                    {
                        itrEv->second->Free();
                        itrEv = m_timeEvents.erase(itrEv);
                    }
                    else
                    {
                        break;
                    }
                }

                auto pEv = m_timeEventPool.Alloc(startTime);
                StoreTimeEvent(pEv);
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
    m_timeEvents[ev->m_time] = ev;
}

void TimeProvider::GetTimeEvents(std::vector<TimeLineEvent*>& ev)
{
    std::lock_guard<MUtilsLockableBase(std::recursive_mutex)> lock(m_mutex);
    ev.resize(m_timeEvents.size());

    uint32_t i = 0;
    for (auto& [t, e] : m_timeEvents)
    {
        ev[i++] = e;
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

} // namespace MUtils

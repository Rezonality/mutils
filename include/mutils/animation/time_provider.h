#pragma once

#include <cstdint>
#include <unordered_set>
#include <mutex>
#include <chrono>

namespace MUtils
{

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
struct TimeEvent
{
    TimePoint time;
};

struct ITimeConsumer
{
    virtual void AddTimeEvent(const TimeEvent& ev) = 0;
};

class TimeProvider
{
public:
    TimeProvider()
    {
        m_startTime = Now();
    }

    static TimeProvider& Instance()
    {
        static TimeProvider provider;
        return provider;
    }

    TimePoint Now() const
    {
        return std::chrono::high_resolution_clock::now();
    }

    TimePoint StartTime() const
    {
        return m_startTime;
    }

    void ResetStartTime()
    {
        m_startTime = Now();
    }

    void Tick(const TimeEvent& ev)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto& consumer : m_consumers)
        {
            consumer->AddTimeEvent(ev);
        }
    }

    void RegisterConsumer(ITimeConsumer* pConsumer)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_consumers.insert(pConsumer);
    }

    void UnRegisterConsumer(ITimeConsumer* pConsumer)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_consumers.erase(pConsumer);
    }

private:
    TimePoint m_startTime;
    std::unordered_set<ITimeConsumer*> m_consumers;
    std::mutex m_mutex;
};

}; // namespace MUtils

#include <algorithm>

#include "mutils/time/time_provider.h"
#include "mutils/profile/profile.h"
#include "mutils/logger/logger.h"

template <typename T, typename Total, size_t N>
class Moving_Average
{
  public:
    void operator()(T sample)
    {
        if (num_samples_ < N)
        {
            samples_[num_samples_++] = sample;
            total_ += sample;
        }
        else
        {
            T& oldest = samples_[num_samples_++ % N];
            total_ += sample - oldest;
            oldest = sample;
        }
    }

    operator double() const { return total_ / std::min(num_samples_, N); }

  private:
    T samples_[N];
    size_t num_samples_{0};
    Total total_{0};
};

namespace MUtils
{

TimeProvider& TimeProvider::Instance()
{
    static TimeProvider provider;
    return provider;
}

TimeProvider::TimeProvider()
{
    m_startTime = Now();
    SetTickRate(120, 4);
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
    std::unique_lock<std::mutex> lock(m_mutex);
    m_consumers.insert(pConsumer);
}

void TimeProvider::UnRegisterConsumer(ITimeConsumer* pConsumer)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_consumers.erase(pConsumer);
}

void TimeProvider::StartThread()
{
    static Moving_Average<uint64_t, uint64_t, 100> av;

    auto lastTime = TimeProvider::Instance().Now();

    // A thread which wakes up and ticks
    m_quitTimer = false;
    m_tickThread = std::thread([&]() {
        for (;;)
        {
            MUtilsZoneScopedN("TimeProvider Tick");
            // Tick at regular intervals, no matter how long our operation takes
            auto startTime = TimeProvider::Instance().Now();
            auto nextTime = startTime + std::chrono::milliseconds(m_timePerBeat);

            av(startTime.time_since_epoch().count() - lastTime.time_since_epoch().count());
            lastTime = startTime;

            auto tick = m_tickCount.load();
            auto beat = tick % m_beatsPerBar.load();
            
            if (tick % 10 == 0)
            {
                LOG(WARNING) << ((long long)(uint64_t)av - std::chrono::nanoseconds(std::chrono::milliseconds(m_timePerBeat)).count());
            }

            TimeEvent ev{ startTime, tick, beat };
            
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                for (auto& consumer : m_consumers)
                {
                    consumer->AddTimeEvent(ev);
                }
            }

            if (m_quitTimer.load())
            {
                break;
            }

            std::this_thread::sleep_until(nextTime);
            m_tickCount++;
        }
    });
}

void TimeProvider::EndThread()
{
    m_quitTimer = true;
    m_tickThread.join();
}

uint32_t TimeProvider::GetTickCount() const
{
    return m_tickCount;
}

uint32_t TimeProvider::GetBeatsPerMinute() const
{
    return m_beatsPerMinute;
}

uint32_t TimeProvider::GetBeat() const
{
    auto tick = m_tickCount.load();
    auto beat = m_beatsPerBar.load();
    return tick % beat;
}


void TimeProvider::SetTickRate(uint32_t bpm, uint32_t beatsPerBar)
{
    m_beatsPerBar = beatsPerBar;
    m_beatsPerMinute = bpm;
    m_timePerBeat = float(60000.0 / (double)(bpm * beatsPerBar));
}

void TimeProvider::SetTickCount(uint32_t tick)
{
    m_tickCount = tick;
}

} // namespace MUtils

/*
Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

/*
CM: Note: Modified from the original to support query of the threads available on the machine,
and fallback to using single threaded if not possible.
Original here: https://github.com/progschj/ThreadPool
*/
#pragma once

// containers
#include <array>
#include <queue>
#include <vector>
// threading
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
// utility wrappers
#include <functional>
#include <memory>
// exceptions
#include <stdexcept>
#include <string>

#include "mutils/profile/profile.h"

#include <concurrentqueue/concurrentqueue.h>
#include <mutils/thread/threadutils.h>

// std::thread pool for resources recycling
class TPool
{
public:
    // the constructor just launches some amount of workers
    TPool(size_t threads_n = std::thread::hardware_concurrency())
        : stop(false)
    {
        // If not enough threads, the pool will just execute all tasks immediately
        if (threads_n > 1)
        {
            this->workers.reserve(threads_n);
            for (; threads_n; --threads_n)
                this->workers.emplace_back(
                    [this] {
#if TRACY_ENABLE
                        tracy::SetThreadName("worker");
#endif
                        while (true)
                        {
                            std::function<void()> task;

                            {
                                std::unique_lock<std::mutex> lock(this->queue_mutex);
                                this->condition.wait(lock,
                                    [this] { return this->stop || !this->tasks.empty(); });
                                if (this->stop && this->tasks.empty())
                                    return;
                                task = std::move(this->tasks.front());
                                this->tasks.pop();
                            }

                            task();
                        }
                    });
        }
    }
    // deleted copy&move ctors&assignments
    TPool(const TPool&) = delete;
    TPool& operator=(const TPool&) = delete;
    TPool(TPool&&) = delete;
    TPool& operator=(TPool&&) = delete;
    // add new work item to the pool
    template <class F, class... Args>
    std::future<typename std::result_of<F(Args...)>::type> enqueue(F&& f, Args&&... args)
    {
        using packaged_task_t = std::packaged_task<typename std::result_of<F(Args...)>::type()>;

        std::shared_ptr<packaged_task_t> task(new packaged_task_t(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)));

        // If there are no works, just run the task in the main thread and return
        if (workers.empty())
        {
            (*task)();
            return task->get_future();
        }
        auto res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->tasks.emplace([task]() { (*task)(); });
        }
        this->condition.notify_one();
        return res;
    }

    void StopAll()
    {
        this->stop = true;
        this->condition.notify_all();
        for (std::thread& worker : this->workers)
            worker.join();
    }

    // the destructor joins all threads
    virtual ~TPool()
    {
        StopAll();
    }

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::queue<std::function<void()>> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    // workers finalization flag
    std::atomic_bool stop;
};

class TAudioThreadPool
{
public:
    // approx. 5x5 ns (= 25 ns), 10x40 ns (= 400 ns), and 3000x350 ns
    // (~ 1 ms), respectively, when measured on a 2.9 GHz Intel i9
    static constexpr std::array iterations = { 5, 10, 3000 };

    bool ShortWait()
    {
        for (int i = 0; i < iterations[0]; ++i)
        {
            if (queue_mutex.try_lock())
            {
                return true;
            }
        }
        return false;
    }

    bool MediumWait()
    {
        for (int i = 0; i < iterations[1]; ++i)
        {
            if (queue_mutex.try_lock())
            {
                return true;
            }

            _mm_pause();
        }
        return false;
    }

    bool LongWait()
    {
        for (int i = 0; i < iterations[2]; ++i)
        {
            if (queue_mutex.try_lock())
                return true;

            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
            _mm_pause();
        }

        // waiting longer than we should, let's give other threads
        // a chance to recover
        std::this_thread::yield();

        return false;
    }

    // the constructor just launches some amount of workers
    TAudioThreadPool(size_t threads_n = std::thread::hardware_concurrency())
        : stop(false)
    {
        // If not enough threads, the pool will just execute all tasks immediately
        if (threads_n > 1)
        {
            this->workers.reserve(threads_n);
            for (; threads_n; --threads_n)
                this->workers.emplace_back(
                    [this] {
#if TRACY_ENABLE
                        tracy::SetThreadName("worker");
#endif
                        // Wait in phases, taking longer each time, but then
                        // going short when we manage it
                        while (true)
                        {
                            switch (waitPhase)
                            {
                            case WaitPhase::Short:
                                if (!ShortWait())
                                {
                                    waitPhase = WaitPhase::Medium;
                                    continue;
                                }
                                break;
                            case WaitPhase::Medium:
                                if (!MediumWait())
                                {
                                    waitPhase = WaitPhase::Long;
                                    continue;
                                }
                                break;
                            case WaitPhase::Long:
                                if (!LongWait())
                                {
                                    continue;
                                }
                                break;
                            }

                            waitPhase = WaitPhase::Short;

                            // This is the finalize/exit path
                            if (this->stop && this->tasks.empty())
                            {
                                queue_mutex.unlock();
                                return;
                            }

                            // Do the new task on this thread
                            std::function<void()> task = std::move(this->tasks.front());
                            this->tasks.pop();

                            // Unlock so that other threads can queue
                            queue_mutex.unlock();

                            // Do the work
                            task();
                        }
                    });
        }
    }

    // deleted copy&move ctors&assignments
    TAudioThreadPool(const TPool&) = delete;
    TAudioThreadPool& operator=(const TPool&) = delete;
    TAudioThreadPool(TPool&&) = delete;
    TAudioThreadPool& operator=(TPool&&) = delete;

    // add new work item to the pool
    template <class F, class... Args>
    std::future<typename std::result_of<F(Args...)>::type> enqueue(F&& f, Args&&... args)
    {
        using packaged_task_t = std::packaged_task<typename std::result_of<F(Args...)>::type()>;

        std::shared_ptr<packaged_task_t> task(new packaged_task_t(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)));

        // If there are no works, just run the task in the main thread and return
        if (workers.empty())
        {
            (*task)();
            return task->get_future();
        }
        auto res = task->get_future();
        {
            // Queue the new task, get the lock to do it; blocks on the spin mutex until it can!
            std::unique_lock<MUtils::audio_spin_mutex> lock(this->queue_mutex);
            this->tasks.emplace([task]() { (*task)(); });
        }
        return res;
    }

    void StopAll()
    {
        this->stop = true;
        for (std::thread& worker : this->workers)
            worker.join();
    }

    // the destructor joins all threads
    virtual ~TAudioThreadPool()
    {
        StopAll();
    }

private:
    enum class WaitPhase
    {
        Short,
        Medium,
        Long
    };

    WaitPhase waitPhase = WaitPhase::Short;

    // All the worker threads
    std::vector<std::thread> workers;

    // the task queue and it's sync mutex
    std::queue<std::function<void()>> tasks;
    MUtils::audio_spin_mutex queue_mutex;

    // workers finalization flag
    std::atomic_bool stop;
};

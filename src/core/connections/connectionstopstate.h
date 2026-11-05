#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <utility>

// One-shot cancellation shared between the owner and the connection worker.
class ConnectionStopState final
{
public:
    bool stopRequested() const { return m_requested.load(std::memory_order_acquire); }

    void requestStop()
    {
        const std::lock_guard lock(m_mutex);
        m_requested.store(true, std::memory_order_release);
        m_condition.notify_all();
    }

    template <typename Ready>
    void waitUntilStopped(Ready &&ready)
    {
        std::unique_lock lock(m_mutex);
        if (stopRequested())
            return;
        std::forward<Ready>(ready)();
        m_condition.wait(lock, [this] { return stopRequested(); });
    }

private:
    std::atomic<bool> m_requested{ false };
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

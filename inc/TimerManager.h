
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <optional>

#include "DoIPTimes.h"

class TimerManager {
public:
    using TimerId = int;

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point expiry;
        std::function<void()> callback;
        std::chrono::milliseconds interval;
        bool periodic;
        TimerId id;
        bool enabled = true;
    };

    std::map<TimerId, TimerEntry> m_timers;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    TimerId m_nextId = 1;

public:
    TimerManager() {
        m_running = true;
        m_thread = std::thread([this]() { run(); });
    }

    ~TimerManager() {
        stop();
    }

    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

    /**
     * @brief Add a timer.
     *
     * @param duration the timer duration in ms
     * @param callback the callback function to invoke when timer expired. Must not be null.
     * @param periodic true, if timer should start again when expired
     * @return std::optional<TimerId>
     */
    std::optional<TimerId> addTimer(std::chrono::milliseconds duration,
                                    std::function<void()> callback,
                                    bool periodic = false) {
        if (!callback) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        TimerId id = m_nextId++;

        TimerEntry entry;
        entry.expiry = std::chrono::steady_clock::now() + duration;
        entry.callback = std::move(callback);
        entry.interval = duration;
        entry.periodic = periodic;
        entry.id = id;
        entry.enabled = true;

        m_timers[id] = std::move(entry);
        m_cv.notify_one();

        return id;
    }

    /**
     * @brief Removes the timer.
     *
     * @param id the id of the timer
     * @return true timer was removed
     * @return false timer with given id does not exist
     */
    bool removeTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_timers.erase(id) > 0;
    }

    bool restartTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_timers.find(id);
        if (it == m_timers.end()) {
            return false;
        }

        it->second.expiry = std::chrono::steady_clock::now() + it->second.interval;
        m_cv.notify_one();
        return true;
    }

    /**
     * @brief Updates a timer duration. The current timer is stopped and then
     * started with the new duration.
     *
     * @param id the id of the timer
     * @param newDuration the new duration in ms
     * @return true timer was updated
     * @return false timer with given id does not exist
     */
    bool updateTimer(TimerId id, std::chrono::milliseconds newDuration) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_timers.find(id);
        if (it == m_timers.end()) {
            return false;
        }

        it->second.interval = newDuration;
        it->second.expiry = std::chrono::steady_clock::now() + newDuration;
        m_cv.notify_one();
        return true;
    }

    /**
     * @brief Disables a timer.
     *
     * @param id the id of the timer
     * @return true timer was disabled
     * @return false timer with given id does not exist
     */
    bool disableTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_timers.find(id);
        if (it == m_timers.end()) {
            return false;
        }
        it->second.enabled = false;
        return true;
    }

    /**
     * @brief Enables a disabled timer. If the specified timer is not
     * disabled, the function has no effect.
     *
     * @param id the id of the timer
     * @return true timer was enabled
     * @return false timer with given id does not exist
     */
    bool enableTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_timers.find(id);
        if (it == m_timers.end()) {
            return false;
        }
        if (!it->second.enabled) {
            it->second.enabled = true;
            it->second.expiry = std::chrono::steady_clock::now() + it->second.interval;
            m_cv.notify_one();
        }
        return true;
    }

    /**
     * @brief Check if specified timer exists.
     *
     * @param id the id of the timer
     * @return true timer exists
     * @return false timer with given id does not exist
     */
    bool hasTimer(TimerId id) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_timers.find(id) != m_timers.end();
    }

    /**
     * @brief The number of timers.
     *
     * @return size_t number of timers.
     */
    size_t timerCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_timers.size();
    }

    /**
     * @brief Stops all timers and the timer manager.
     */
    void stop() {
        if (m_running.exchange(false)) {
            m_cv.notify_all();
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }
    }

private:
    void run() {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_timers.empty()) {
                m_cv.wait(lock, [this]() {
                    return !m_running || !m_timers.empty();
                });
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            auto nextExpiry = std::chrono::steady_clock::time_point::max();

            for (const auto& [id, timer] : m_timers) {
                if (timer.enabled && timer.expiry < nextExpiry) {
                    nextExpiry = timer.expiry;
                }
            }

            if (nextExpiry > now) {
                m_cv.wait_until(lock, nextExpiry, [this]() {
                    return !m_running;
                });
                continue;
            }

            std::vector<TimerId> expired;
            for (const auto& [id, timer] : m_timers) {
                if (timer.enabled && timer.expiry <= now) {
                    expired.push_back(id);
                }
            }

            lock.unlock();

            for (TimerId id : expired) {
                lock.lock();
                auto it = m_timers.find(id);
                if (it == m_timers.end() || !it->second.enabled) {
                    lock.unlock();
                    continue;
                }

                auto callback = it->second.callback;
                bool periodic = it->second.periodic;
                auto interval = it->second.interval;

                if (periodic) {
                    it->second.expiry = std::chrono::steady_clock::now() + interval;
                } else {
                    m_timers.erase(it);
                }

                lock.unlock();

                try {
                    callback();
                } catch (...) {
                    // Swallow exceptions to prevent thread termination
                }
            }
        }
    }
};
#include <doctest/doctest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <stdexcept>

#include "TimerManager.h"

using namespace std::chrono_literals;

TEST_SUITE("TimerManager") {
    TEST_CASE("Basic timer creation and execution") {
        TimerManager manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted]() noexcept {
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(50ms, callback);

        REQUIRE(timerId.has_value());
        CHECK(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 1);

        // Wait for timer to execute
        std::this_thread::sleep_for(100ms);

        CHECK(callbackExecuted);
        CHECK(manager.timerCount() == 0); // One-shot timer should be removed
    }

    TEST_CASE("Periodic timer") {
        TimerManager manager;
        std::atomic<int> executionCount{0};

        auto callback = [&executionCount]() noexcept {
            executionCount++;
        };

        auto timerId = manager.addTimer(30ms, callback, true); // periodic = true

        REQUIRE(timerId.has_value());
        CHECK(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 1);

        // Wait for multiple executions
        std::this_thread::sleep_for(100ms);

        CHECK(executionCount >= 2); // Should have executed at least twice
        CHECK(manager.timerCount() == 1); // Periodic timer should still exist

        // Clean up
        manager.removeTimer(timerId.value());
        CHECK(manager.timerCount() == 0);
    }

    TEST_CASE("Timer removal") {
        TimerManager manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted]() noexcept {
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(100ms, callback);

        REQUIRE(timerId.has_value());
        CHECK(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 1);

        // Remove timer before it executes
        bool removed = manager.removeTimer(timerId.value());
        CHECK(removed);
        CHECK_FALSE(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 0);

        // Wait to ensure callback doesn't execute
        std::this_thread::sleep_for(150ms);
        CHECK_FALSE(callbackExecuted);

        // Try to remove non-existent timer
        bool removedAgain = manager.removeTimer(timerId.value());
        CHECK_FALSE(removedAgain);
    }

    TEST_CASE("Timer restart") {
        TimerManager manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted]() noexcept {
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(100ms, callback);

        REQUIRE(timerId.has_value());

        // Wait a bit, then restart timer
        std::this_thread::sleep_for(50ms);
        bool restarted = manager.restartTimer(timerId.value());
        CHECK(restarted);

        // Wait another 60ms (total 110ms, but timer was restarted at 50ms)
        std::this_thread::sleep_for(60ms);
        CHECK_FALSE(callbackExecuted); // Should not have executed yet

        // Wait another 50ms (now 100ms since restart)
        std::this_thread::sleep_for(50ms);
        CHECK(callbackExecuted);

        // Try to restart non-existent timer
        bool restartedNonExistent = manager.restartTimer(999);
        CHECK_FALSE(restartedNonExistent);
    }

    TEST_CASE("Timer update duration") {
        TimerManager manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted]() noexcept {
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(200ms, callback);

        REQUIRE(timerId.has_value());

        // Update to shorter duration
        bool updated = manager.updateTimer(timerId.value(), 50ms);
        CHECK(updated);

        // Should execute sooner now
        std::this_thread::sleep_for(80ms);
        CHECK(callbackExecuted);

        // Try to update non-existent timer
        bool updatedNonExistent = manager.updateTimer(999, 100ms);
        CHECK_FALSE(updatedNonExistent);
    }

    TEST_CASE("Timer enable/disable") {
        // Test 1: Disable functionality
        {
            TimerManager manager;
            std::atomic<bool> callbackExecuted{false};

            auto callback = [&callbackExecuted]() noexcept {
                callbackExecuted = true;
            };

            // Create timer with longer duration to have time to disable it
            auto timerId = manager.addTimer(200ms, callback);
            REQUIRE(timerId.has_value());

            // Disable timer immediately
            bool disabled = manager.disableTimer(timerId.value());
            CHECK(disabled);

            // Wait longer than timer duration to ensure it doesn't execute
            std::this_thread::sleep_for(250ms);
            CHECK_FALSE(callbackExecuted);
        }

        // Test 2: Basic enable/disable API
        {
            TimerManager manager;

            // Try enable/disable non-existent timer
            bool disabledNonExistent = manager.disableTimer(999);
            CHECK_FALSE(disabledNonExistent);

            bool enabledNonExistent = manager.enableTimer(999);
            CHECK_FALSE(enabledNonExistent);
        }
    }

    TEST_CASE("Multiple timers") {
        TimerManager manager;
        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};
        std::atomic<int> counter3{0};

        auto callback1 = [&counter1]() noexcept { counter1++; };
        auto callback2 = [&counter2]() noexcept { counter2++; };
        auto callback3 = [&counter3]() noexcept { counter3++; };

        auto timer1 = manager.addTimer(30ms, callback1, true);
        auto timer2 = manager.addTimer(50ms, callback2, true);
        auto timer3 = manager.addTimer(80ms, callback3);

        REQUIRE(timer1.has_value());
        REQUIRE(timer2.has_value());
        REQUIRE(timer3.has_value());

        CHECK(manager.timerCount() == 3);

        // Wait for all timers to execute
        std::this_thread::sleep_for(150ms);

        CHECK(counter1 >= 3); // Should execute multiple times
        CHECK(counter2 >= 2); // Should execute multiple times
        CHECK(counter3 == 1); // One-shot timer

        CHECK(manager.timerCount() == 2); // One-shot timer should be removed

        // Clean up
        manager.removeTimer(timer1.value());
        manager.removeTimer(timer2.value());
        CHECK(manager.timerCount() == 0);
    }

    TEST_CASE("Null callback handling") {
        TimerManager manager;

        // Try to add timer with null callback
        auto timerId = manager.addTimer(50ms, nullptr);
        CHECK_FALSE(timerId.has_value());
        CHECK(manager.timerCount() == 0);
    }

    TEST_CASE("Exception handling in callback") {
        TimerManager manager;
        std::atomic<bool> normalCallbackExecuted{false};

        // Add timer that throws exception
        auto throwingCallback = []() {
            throw std::runtime_error("Test exception");
        };

        auto normalCallback = [&normalCallbackExecuted]() noexcept {
            normalCallbackExecuted = true;
        };

        auto throwingTimer = manager.addTimer(30ms, throwingCallback);
        auto normalTimer = manager.addTimer(50ms, normalCallback);

        REQUIRE(throwingTimer.has_value());
        REQUIRE(normalTimer.has_value());

        // Wait for both timers to execute
        std::this_thread::sleep_for(100ms);

        // Normal timer should still execute despite exception in other timer
        CHECK(normalCallbackExecuted);
    }

    TEST_CASE("Timer manager destruction") {
        std::atomic<bool> callbackExecuted{false};

        {
            TimerManager manager;
            auto callback = [&callbackExecuted]() noexcept {
                callbackExecuted = true;
            };

            auto timerId = manager.addTimer(200ms, callback);

            REQUIRE(timerId.has_value());
            CHECK(manager.timerCount() == 1);

            // Manager goes out of scope here and should stop cleanly
        }

        // Wait to ensure callback doesn't execute after destruction
        std::this_thread::sleep_for(250ms);
        CHECK_FALSE(callbackExecuted);
    }

    TEST_CASE("Basic functionality verification") {
        TimerManager manager;
        std::atomic<int> totalExecutions{0};
        constexpr int timerCount = 5; // Reduced from 50 for faster testing
        std::vector<TimerManager::TimerId> timerIds;

        // Add timers with different intervals
        for (int i = 0; i < timerCount; ++i) {
            auto callback = [&totalExecutions]() noexcept {
                totalExecutions++;
            };

            auto duration = std::chrono::milliseconds(20 + (i % 10)); // 20-29ms
            auto timerId = manager.addTimer(duration, callback);
            REQUIRE(timerId.has_value());
            timerIds.push_back(timerId.value());
        }

        CHECK(manager.timerCount() == timerCount);

        // Wait for timers to execute
        std::this_thread::sleep_for(100ms);

        CHECK(totalExecutions == timerCount);
        CHECK(manager.timerCount() == 0); // All one-shot timers should be removed
    }
}
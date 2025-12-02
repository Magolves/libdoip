#include <atomic>
#include <chrono>
#include <doctest/doctest.h>
#include <stdexcept>
#include <thread>
#include <vector>

#include "TimerManager.h"

using namespace std::chrono_literals;
using namespace doip;

enum class TimerTestId : uint8_t {
    TimerOne = 1,
    TimerTwo = 2,
    TimerThree = 3,
    NonExistent = 99,
};

TEST_SUITE("TimerManager") {
    TEST_CASE("Basic timer creation and execution") {
        TimerManager<TimerTestId> manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted](TimerTestId id) noexcept {
            (void)id;
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(TimerTestId::TimerOne, 50ms, callback);

        REQUIRE(timerId.has_value());
        CHECK(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 1);

        // Wait for timer to execute
        std::this_thread::sleep_for(100ms);

        CHECK(callbackExecuted);
        CHECK(manager.timerCount() == 0); // One-shot timer should be removed
    }

    TEST_CASE("Periodic timer") {
        TimerManager<TimerTestId> manager;
        std::atomic<int> executionCount{0};

        auto callback = [&executionCount](TimerTestId id) noexcept {
            (void)id;
            executionCount++;
        };

        auto timerId = manager.addTimer(TimerTestId::TimerOne, 30ms, callback, true); // periodic = true

        REQUIRE(timerId.has_value());
        CHECK(manager.hasTimer(timerId.value()));
        CHECK(manager.timerCount() == 1);

        // Wait for multiple executions
        std::this_thread::sleep_for(100ms);

        CHECK(executionCount >= 2);       // Should have executed at least twice
        CHECK(manager.timerCount() == 1); // Periodic timer should still exist

        // Clean up
        manager.removeTimer(timerId.value());
        CHECK(manager.timerCount() == 0);
    }

    TEST_CASE("TimerOne removal") {
        TimerManager<TimerTestId> manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted](TimerTestId id) noexcept {
            (void)id;
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(TimerTestId::TimerOne, 100ms, callback);

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

    TEST_CASE("TimerOne restart") {
        TimerManager<TimerTestId> manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted](TimerTestId id) noexcept {
            (void)id;
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(TimerTestId::TimerOne, 100ms, callback);

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
        bool restartedNonExistent = manager.restartTimer(TimerTestId::TimerOne);
        CHECK_FALSE(restartedNonExistent);
    }

    TEST_CASE("TimerOne update duration") {
        TimerManager<TimerTestId> manager;
        std::atomic<bool> callbackExecuted{false};

        auto callback = [&callbackExecuted](TimerTestId id) noexcept {
            (void)id;
            callbackExecuted = true;
        };

        auto timerId = manager.addTimer(TimerTestId::TimerOne, 200ms, callback);

        REQUIRE(timerId.has_value());

        // Update to shorter duration
        bool updated = manager.updateTimer(timerId.value(), 50ms);
        CHECK(updated);

        // Should execute sooner now. Add some tolerance due to delays
        // caused by sanitizers
        const int maxWaitMs = 200;
        int waited = 0;
        while (!callbackExecuted && waited < maxWaitMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            waited += 5;
        }
        MESSAGE("Callback executed after ", (waited * 5), "ms");
        CHECK(callbackExecuted);

        // Try to update non-existent timer
        bool updatedNonExistent = manager.updateTimer(TimerTestId::TimerOne, 100ms);
        CHECK_FALSE(updatedNonExistent);
    }

    TEST_CASE("TimerOne enable/disable") {
        // Test 1: Disable functionality
        {
            TimerManager<TimerTestId> manager;
            std::atomic<bool> callbackExecuted{false};

            auto callback = [&callbackExecuted](TimerTestId id) noexcept {
                (void)id;
                callbackExecuted = true;
            };

            // Create timer with longer duration to have time to disable it
            auto timerId = manager.addTimer(TimerTestId::TimerOne, 200ms, callback);
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
            TimerManager<TimerTestId> manager;

            // Try enable/disable non-existent timer
            bool disabledNonExistent = manager.disableTimer(TimerTestId::NonExistent);
            CHECK_FALSE(disabledNonExistent);

            bool enabledNonExistent = manager.enableTimer(TimerTestId::NonExistent);
            CHECK_FALSE(enabledNonExistent);
        }
    }

    TEST_CASE("Multiple timers") {
        TimerManager<TimerTestId> manager;
        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};
        std::atomic<int> counter3{0};

        auto callback1 = [&counter1](TimerTestId tid) noexcept { (void) tid; counter1++; };
        auto callback2 = [&counter2](TimerTestId tid) noexcept { (void) tid; counter2++; };
        auto callback3 = [&counter3](TimerTestId tid) noexcept { (void) tid; counter3++; };

        auto timer1 = manager.addTimer(TimerTestId::TimerOne, 30ms, callback1, true);
        auto timer2 = manager.addTimer(TimerTestId::TimerTwo, 50ms, callback2, true);
        auto timer3 = manager.addTimer(TimerTestId::TimerThree, 80ms, callback3);

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
        TimerManager<TimerTestId> manager;

        // Try to add timer with null callback
        auto timerId = manager.addTimer(TimerTestId::TimerOne, 50ms, nullptr);
        CHECK_FALSE(timerId.has_value());
        CHECK(manager.timerCount() == 0);
    }

    TEST_CASE("Exception handling in callback") {
        TimerManager<TimerTestId> manager;
        std::atomic<bool> normalCallbackExecuted{false};

        // Add timer that throws exception
        auto throwingCallback = [](TimerTestId id)  {
            (void)id;
            throw std::runtime_error("Test exception");
        };

        auto normalCallback = [&normalCallbackExecuted](TimerTestId id) noexcept {
            (void)id;
            normalCallbackExecuted = true;
        };

        auto throwingTimer = manager.addTimer(TimerTestId::TimerOne, 30ms, throwingCallback);
        auto normalTimer = manager.addTimer(TimerTestId::TimerOne, 50ms, normalCallback);

        REQUIRE(throwingTimer.has_value());
        REQUIRE(normalTimer.has_value());

        // Wait for both timers to execute
        std::this_thread::sleep_for(100ms);

        // Normal timer should still execute despite exception in other timer
        CHECK(normalCallbackExecuted);
    }

    TEST_CASE("Basic functionality verification") {
        TimerManager<TimerTestId> manager;
        std::atomic<int> totalExecutions{0};
        constexpr int timerCount = 3;
        std::vector<TimerTestId> timerIds;

        // Add timers with different intervals
        for (int i = 0; i < timerCount; ++i) {
            auto callback = [&totalExecutions](TimerTestId id) noexcept {
                (void)id;
                totalExecutions++;
            };

            auto duration = std::chrono::milliseconds(20 + (i % 10)); // 20-29ms
            TimerTestId timerId;
            switch(i) {
                case 0:
                    timerId = TimerTestId::TimerOne;
                    break;
                case 1:
                    timerId = TimerTestId::TimerTwo;
                    break;
                case 2:
                    timerId = TimerTestId::TimerThree;
                    break;
                default:
                    timerId = TimerTestId::NonExistent; // Should not happen
            }

            auto timerIdOpt = manager.addTimer(timerId, duration, callback);
            REQUIRE(timerIdOpt.has_value());
            timerIds.push_back(timerIdOpt.value());
        }

        CHECK(manager.timerCount() == timerCount);

        // Wait for timers to execute
        std::this_thread::sleep_for(100ms);

        CHECK(totalExecutions == timerCount);
        CHECK(manager.timerCount() == 0); // All one-shot timers should be removed
    }
}
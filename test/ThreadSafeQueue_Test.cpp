#include <doctest/doctest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <future>

#include "ThreadSafeQueue.h"

using namespace std::chrono_literals;

TEST_SUITE("ThreadSafeQueue") {
    TEST_CASE("Basic push and pop operations") {
        ThreadSafeQueue<int> queue;

        // Test size
        CHECK(queue.size() == 0);

        // Push some items
        queue.push(1);
        queue.push(2);
        queue.push(3);

        CHECK(queue.size() == 3);

        // Pop items in FIFO order
        int item;
        bool success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == 1);
        CHECK(queue.size() == 2);

        success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == 2);
        CHECK(queue.size() == 1);

        success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == 3);
        CHECK(queue.size() == 0);
    }

    TEST_CASE("Pop timeout on empty queue") {
        ThreadSafeQueue<int> queue;

        int item;
        auto start = std::chrono::steady_clock::now();
        bool success = queue.pop(item, 50ms);
        auto end = std::chrono::steady_clock::now();

        CHECK_FALSE(success);

        // Check that timeout was respected (with some tolerance)
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        CHECK(elapsed >= 40ms); // Allow some tolerance for timing
        CHECK(elapsed <= 100ms); // But not too much over
    }

    TEST_CASE("Thread safety - multiple producers single consumer") {
        ThreadSafeQueue<int> queue;
        std::atomic<int> producedCount{0};
        std::atomic<int> consumedCount{0};
        constexpr int itemsPerProducer = 100;
        constexpr int numProducers = 4;
        constexpr int totalItems = itemsPerProducer * numProducers;

        // Start producer threads
        std::vector<std::thread> producers;
        for (int i = 0; i < numProducers; ++i) {
            producers.emplace_back([&queue, &producedCount, i]() {
                for (int j = 0; j < itemsPerProducer; ++j) {
                    queue.push(i * itemsPerProducer + j);
                    producedCount++;
                }
            });
        }

        // Start consumer thread
        std::thread consumer([&queue, &consumedCount]() {
            int item;
            while (consumedCount < totalItems) {
                if (queue.pop(item, 10ms)) {
                    consumedCount++;
                }
            }
        });

        // Wait for all threads to complete
        for (auto& producer : producers) {
            producer.join();
        }
        consumer.join();

        CHECK(producedCount == totalItems);
        CHECK(consumedCount == totalItems);
        CHECK(queue.size() == 0);
    }

    TEST_CASE("Thread safety - single producer multiple consumers") {
        ThreadSafeQueue<int> queue;
        std::atomic<int> producedCount{0};
        std::atomic<int> consumedCount{0};
        constexpr int totalItems = 400;
        constexpr int numConsumers = 4;

        // Start producer thread
        std::thread producer([&queue, &producedCount]() {
            for (int i = 0; i < totalItems; ++i) {
                queue.push(i);
                producedCount++;
                // Small delay to make race conditions more likely
                std::this_thread::sleep_for(1us);
            }
        });

        // Start consumer threads
        std::vector<std::thread> consumers;
        for (int i = 0; i < numConsumers; ++i) {
            consumers.emplace_back([&queue, &consumedCount]() {
                int item;
                while (true) {
                    if (queue.pop(item, 10ms)) {
                        consumedCount++;
                    } else {
                        // Timeout - check if we're done
                        if (consumedCount >= 400) break;
                    }
                }
            });
        }

        // Wait for producer to finish
        producer.join();

        // Wait for all items to be consumed
        while (consumedCount < totalItems) {
            std::this_thread::sleep_for(1ms);
        }

        // Stop consumers
        for (auto& consumer : consumers) {
            consumer.join();
        }

        CHECK(producedCount == totalItems);
        CHECK(consumedCount == totalItems);
    }

    TEST_CASE("Stop functionality") {
        ThreadSafeQueue<int> queue;

        // Add some items
        queue.push(1);
        queue.push(2);
        CHECK(queue.size() == 2);

        // Stop the queue
        queue.stop();

        // Try to push after stop - should be ignored
        queue.push(3);
        CHECK(queue.size() == 2); // Should still be 2

        // Pop should still work for existing items
        int item;
        bool success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == 1);

        success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == 2);

        // Pop from empty stopped queue should return false immediately
        auto start = std::chrono::steady_clock::now();
        success = queue.pop(item, 100ms);
        auto end = std::chrono::steady_clock::now();

        CHECK_FALSE(success);

        // Should return quickly, not wait for timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        CHECK(elapsed < 50ms);
    }

    TEST_CASE("Stop with waiting consumers") {
        ThreadSafeQueue<int> queue;
        std::atomic<bool> consumerFinished{false};
        std::atomic<bool> popResult{false};

        // Start a consumer that will wait
        std::thread consumer([&queue, &consumerFinished, &popResult]() {
            int item;
            popResult = queue.pop(item, 1000ms); // Long timeout
            consumerFinished = true;
        });

        // Give consumer time to start waiting
        std::this_thread::sleep_for(50ms);
        CHECK_FALSE(consumerFinished);

        // Stop the queue - should wake up waiting consumer
        queue.stop();

        // Consumer should finish quickly
        consumer.join();
        CHECK(consumerFinished);
        CHECK_FALSE(popResult); // Should return false due to stop
    }

    TEST_CASE("Move semantics") {
        ThreadSafeQueue<std::unique_ptr<int>> queue;

        // Push unique_ptr (move semantics)
        queue.push(std::make_unique<int>(42));
        queue.push(std::make_unique<int>(24));

        CHECK(queue.size() == 2);

        // Pop unique_ptr
        std::unique_ptr<int> item;
        bool success = queue.pop(item, 100ms);
        CHECK(success);
        REQUIRE(item != nullptr);
        CHECK(*item == 42);

        success = queue.pop(item, 100ms);
        CHECK(success);
        REQUIRE(item != nullptr);
        CHECK(*item == 24);

        CHECK(queue.size() == 0);
    }

    TEST_CASE("String operations") {
        ThreadSafeQueue<std::string> queue;

        queue.push("Hello");
        queue.push("World");
        queue.push("Test");

        CHECK(queue.size() == 3);

        std::string item;
        bool success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == "Hello");

        success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == "World");

        success = queue.pop(item, 100ms);
        CHECK(success);
        CHECK(item == "Test");

        CHECK(queue.size() == 0);
    }

    TEST_CASE("Large number of items") {
        ThreadSafeQueue<int> queue;
        constexpr int numItems = 10000;

        // Push many items
        for (int i = 0; i < numItems; ++i) {
            queue.push(i);
        }

        CHECK(queue.size() == numItems);

        // Pop all items and verify order
        for (int i = 0; i < numItems; ++i) {
            int item;
            bool success = queue.pop(item, 100ms);
            CHECK(success);
            CHECK(item == i);
        }

        CHECK(queue.size() == 0);
    }

    TEST_CASE("Concurrent push and pop") {
        ThreadSafeQueue<int> queue;
        std::atomic<bool> stopFlag{false};
        std::atomic<int> pushCount{0};
        std::atomic<int> popCount{0};

        // Producer thread
        std::thread producer([&]() {
            int value = 0;
            while (!stopFlag) {
                queue.push(value++);
                pushCount++;
                if (pushCount >= 1000) break;
                std::this_thread::sleep_for(1us);
            }
        });

        // Consumer thread
        std::thread consumer([&]() {
            int item;
            while (!stopFlag) {
                if (queue.pop(item, 10ms)) {
                    popCount++;
                }
                if (popCount >= 1000) break;
            }
        });

        // Let them run for a bit
        std::this_thread::sleep_for(100ms);
        stopFlag = true;

        producer.join();
        consumer.join();

        // Drain remaining items
        int item;
        while (queue.pop(item, 10ms)) {
            popCount++;
        }

        CHECK(pushCount == popCount);
    }

    TEST_CASE("Zero timeout behavior") {
        ThreadSafeQueue<int> queue;

        // Pop from empty queue with zero timeout should return immediately
        int item;
        auto start = std::chrono::steady_clock::now();
        bool success = queue.pop(item, 0ms);
        auto end = std::chrono::steady_clock::now();

        CHECK_FALSE(success);

        // Should be very fast
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        CHECK(elapsed < 10ms);

        // Add item and try again - should succeed immediately
        queue.push(42);
        success = queue.pop(item, 0ms);
        CHECK(success);
        CHECK(item == 42);
    }
}
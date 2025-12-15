#pragma once
#include "ByteArray.h"
#include "IDownstreamProvider.h"
#include "uds/UdsMock.h"
#include <thread>
#include <atomic>
#include "ThreadSafeQueue.h"

namespace doip::uds {

using namespace std::chrono_literals;
using namespace doip;

class UdsMockProvider : public IDownstreamProvider {
public:
    UdsMockProvider()
        : m_running(false)
    {
        m_uds.registerDefaultServices();
    }

    ~UdsMockProvider() override {
        stop();
    }

    void start() override {
        m_running = true;
        m_worker = std::thread([this]{ workerLoop(); });
    }

    void stop() override {
        m_running = false;
        if (m_worker.joinable()) m_worker.join();
    }

    void sendRequest(const ByteArray request, DownstreamCallback cb) override {
        if (!cb) return;

        DownstreamTask task;
        task.request = std::move(request);
        task.cb = std::move(cb);

        m_tasks.push(task);
    }

private:
    struct DownstreamTask {
        ByteArray request;
        DownstreamCallback cb;
    };

    std::atomic<bool> m_running;
    std::thread m_worker;
    ThreadSafeQueue<DownstreamTask> m_tasks;

    uds::UdsMock m_uds;

    void workerLoop() {
        while (m_running) {
            DownstreamTask task;
            if (m_tasks.pop(task)) {
                auto start_ts = std::chrono::steady_clock::now();

                // Synchronous UDS processing
                ByteArray rsp = m_uds.handleDiagnosticRequest(task.request);

                auto end_ts = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts);

                DownstreamResponse dr;
                dr.payload = rsp;
                dr.latency = latency;
                dr.status = DownstreamStatus::Handled;

                task.cb(dr);
            }

            std::this_thread::sleep_for(1ms);
        }
    }
};

} // namespace doip::uds

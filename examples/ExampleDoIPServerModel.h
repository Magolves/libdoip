/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef EXAMPLEDOIPSERVERMODEL_H
#define EXAMPLEDOIPSERVERMODEL_H

#include "DoIPServerModel.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMock.h"

using namespace doip;

class ExampleDoIPServerModel : public DoIPServerModel {
  public:
    ExampleDoIPServerModel() {
        onOpenConnection = [this](IConnectionContext &ctx) noexcept {
            (void)ctx;
            startWorker();
        };
        onCloseConnection = [this](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
            stopWorker();
            DOIP_LOG_WARN("Connection closed ({})", fmt::streamed(reason));
        };

        onDiagnosticMessage = [this](IConnectionContext &ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            m_log->info("Received Diagnostic message (from ExampleDoIPServerModel)", fmt::streamed(msg));

            // Example: Access payload using getPayload()
            auto payload = msg.getDiagnosticMessagePayload();
            if (payload.second >= 3 && payload.first[0] == 0x22 && payload.first[1] == 0xF1 && payload.first[2] == 0x90) {
                m_log->info(" - Detected Read Data by Identifier for VIN (0xF190) -> send NACK");
                return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
            }

            return std::nullopt;
        };

        onDiagnosticNotification = [this](IConnectionContext &ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            m_log->info("Diagnostic ACK/NACK sent (from ExampleDoIPServerModel)", fmt::streamed(ack));
        };

        onDownstreamRequest = [this](IConnectionContext &ctx, const DoIPMessage &msg, ServerModelDownstreamResponseHandler callback) noexcept {
            (void)ctx;
            (void)msg;

            m_log->info("Received downstream request (from ExampleDoIPServerModel)", fmt::streamed(msg));
            m_downstreamCallback = callback;
            if (!m_downstreamCallback) {
                m_log->error("onDownstreamRequest: No callback function passed");
                return DoIPDownstreamResult::Error;
            }

            // Store message in send queue
            auto [data, size] = msg.getDiagnosticMessagePayload();
            m_tx.push(ByteArray(data, size));
            m_log->info("Enqueued msg");
            return DoIPDownstreamResult::Pending;
        };

    }

  private:
    std::shared_ptr<spdlog::logger> m_log = Logger::get("smodel");
    ServerModelDownstreamResponseHandler m_downstreamCallback = nullptr;
    ThreadSafeQueue<ByteArray> m_rx;
    ThreadSafeQueue<ByteArray> m_tx;
    uds::UdsMock m_uds;
    std::thread m_worker;
    bool m_running = true;

    void startWorker() {
        m_worker = std::thread([this] {
            while (m_running) {
                downstream_thread();
            }
        });
        m_log->info("Started worker thread");
    }
    void stopWorker() {
        m_running = false;
        if (m_worker.joinable())
            m_worker.join();
        m_log->info("Stopped worker thread");
    }

    /**
     * @brief Thread simulating downstream communication (e. g. CAN).
     */
    void downstream_thread() {
        if (m_tx.size()) {
            // simulate send. In a real environment we could send a CAN message
            ByteArray req;
            m_tx.pop(req);
            m_log->info("Simulate send {}", fmt::streamed(req));
            // simulate some latency
            std::this_thread::sleep_for(50ms);
            // simulate receive
            m_rx.push(m_uds.handleDiagnosticRequest(req));
        }

        if (m_rx.size()) {
            ByteArray rsp;
            m_rx.pop(rsp);
            m_log->info("Simulate receive {}", fmt::streamed(rsp));
            if (m_downstreamCallback) {
                m_downstreamCallback(rsp, DoIPDownstreamResult::Handled);
                m_downstreamCallback = nullptr;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */

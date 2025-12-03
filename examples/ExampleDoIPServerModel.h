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
        onCloseConnection = [this](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
            stopWorker();
            DOIP_LOG_WARN("Connection closed ({})", fmt::streamed(reason));
        };

        onDiagnosticMessage = [](IConnectionContext &ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            DOIP_LOG_INFO("Received Diagnostic message (from ExampleDoIPServerModel)", fmt::streamed(msg));

            // Example: Access payload using getPayload()
            auto payload = msg.getDiagnosticMessagePayload();
            if (payload.second >= 3 && payload.first[0] == 0x22 && payload.first[1] == 0xF1 && payload.first[2] == 0x90) {
                DOIP_LOG_INFO(" - Detected Read Data by Identifier for VIN (0xF190) -> send NACK");
                return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
            }

            return std::nullopt;
        };

        onDiagnosticNotification = [](IConnectionContext &ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            DOIP_LOG_INFO("Diagnostic ACK/NACK sent (from ExampleDoIPServerModel)", fmt::streamed(ack));
        };

        onDownstreamRequest = [this](IConnectionContext &ctx, const DoIPMessage &msg, ServerModelDownstreamResponseHandler callback) noexcept {
            (void)ctx;
            (void)msg;
            // Store message in send queue
            // auto [data, size] = msg.getDiagnosticMessagePayload();
            // m_tx.push(ByteArray(data, size));
            m_downstreamCallback = callback;
            if (!m_downstreamCallback) {
                DOIP_LOG_ERROR("onDownstreamRequest: No callback function passed");
                return DoIPDownstreamResult::Error;
            }
            return DoIPDownstreamResult::Pending;
        };

        startWorker();
    }

  private:
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
    }
    void stopWorker() {
        m_running = false;
        if (m_worker.joinable())
            m_worker.join();
        DOIP_LOG_INFO("Stopped worker thread");
    }

    /**
     * @brief Thread simulating downstream communication (e. g. CAN).
     */
    void downstream_thread() {
        if (m_tx.size()) {
            // simulate send. In a real environment we could send a CAN message
            ByteArray req;
            m_tx.pop(req);
            sleep(1); // wait for response
            // simulate receive
            m_rx.push(m_uds.handleDiagnosticRequest(req));
        }

        if (m_rx.size()) {
            ByteArray rsp;
            m_rx.pop(rsp);
            if (m_downstreamCallback) {
                m_downstreamCallback(rsp, DoIPDownstreamResult::Handled);
                m_downstreamCallback = nullptr;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */

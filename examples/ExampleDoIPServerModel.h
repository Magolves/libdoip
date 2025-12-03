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
        onCloseConnection = [](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
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
            return DoIPDownstreamResult::Pending;
        };
    }

    private:
        ServerModelDownstreamResponseHandler m_downstreamCallback = nullptr;
        ThreadSafeQueue<ByteArray> m_rx;
        ThreadSafeQueue<ByteArray> m_tx;
        uds::UdsMock uds;


        void downstream_thread() {
            if (m_tx.size()) {
                // simulate send
                ByteArray req;
                m_tx.pop(req);
                sleep(1);
                m_rx.push(uds.handleDiagnosticRequest(req));
            }

            if (m_rx.size()) {
                ByteArray rsp;
                m_rx.pop(rsp);
                if (m_downstreamCallback) {
                    m_downstreamCallback(rsp, DoIPDownstreamResult::Handled);
                }
            }
            std::this_thread::sleep_for(10ms);
        }
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */

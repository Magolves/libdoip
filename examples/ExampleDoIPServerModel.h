/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef EXAMPLEDOIPSERVERMODEL_H
#define EXAMPLEDOIPSERVERMODEL_H

#include "DoIPServerModel.h"

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
    }
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */

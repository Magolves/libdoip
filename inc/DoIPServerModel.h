#ifndef DOIPSERVERMODEL_H
#define DOIPSERVERMODEL_H

#include <functional>

#include "DoIPAddress.h"
#include "DoIPServerEvent.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerState.h"
#include "DoIPCloseReason.h"
#include "Logger.h"

namespace doip {

// Forward declaration
class IConnectionContext;

using ServerModelCloseHandler = std::function<void(IConnectionContext&, DoIPCloseReason)>;
using ServerModelDiagnosticHandler = std::function<DoIPDiagnosticAck(IConnectionContext&, const DoIPMessage &)>;
using ServerModelDiagnosticNotificationHandler = std::function<void(IConnectionContext&, DoIPDiagnosticAck)>;

struct DoIPServerModel {

    ServerModelCloseHandler onCloseConnection;
    ServerModelDiagnosticHandler onDiagnosticMessage;
    ServerModelDiagnosticNotificationHandler onDiagnosticNotification;

    DoIPAddress serverAddress;
};

using UniqueServerModelPtr = std::unique_ptr<DoIPServerModel>;

struct DefaultDoIPServerModel : public DoIPServerModel {
    DefaultDoIPServerModel() {
        DOIP_LOG_CRITICAL("Using DefaultDoIPServerModel - no callbacks are set!");

        onCloseConnection = [](IConnectionContext& ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
            DOIP_LOG_CRITICAL("Close connection called on DefaultDoIPServerModel - reason {}", fmt::streamed(reason));
            // Default no-op
        };
        onDiagnosticMessage = [](IConnectionContext& ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            (void)msg;
            DOIP_LOG_CRITICAL("Diagnostic message received on DefaultDoIPServerModel");
            // Default: always ACK
            return std::nullopt;
        };
        onDiagnosticNotification = [](IConnectionContext& ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            (void)ack;
            DOIP_LOG_CRITICAL("Diagnostic message received on DefaultDoIPServerModel");
            // Default no-op
        };
    }
};

} // namespace doip

#endif /* DOIPSERVERMODEL_H */

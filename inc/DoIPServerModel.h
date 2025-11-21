#ifndef DOIPSERVERMODEL_H
#define DOIPSERVERMODEL_H

#include <functional>

#include "DoIPAddress.h"
#include "DoIPEvent.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPState.h"

namespace doip {

using CloseConnectionHandler = std::function<void()>;
using DiagnosticMessageHandler = std::function<DoIPDiagnosticAck(const DoIPMessage &)>;
using DiagnosticNotificationHandler = std::function<void(DoIPDiagnosticAck)>;

struct DoIPServerModel {

    CloseConnectionHandler onCloseConnection;
    DiagnosticMessageHandler onDiagnosticMessage;
    DiagnosticNotificationHandler onDiagnosticNotification;

    DoIPAddress serverAddress;
};

struct DefaultDoIPServerModel : public DoIPServerModel {
    DefaultDoIPServerModel() {
        onCloseConnection = []() noexcept {
            // Default no-op
        };
        onDiagnosticMessage = [](const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)msg;
            // Default: always ACK
            return std::nullopt;
        };
        onDiagnosticNotification = [](DoIPDiagnosticAck ack) noexcept {
            (void)ack;
            // Default no-op
        };
    }
};

} // namespace doip

#endif /* DOIPSERVERMODEL_H */

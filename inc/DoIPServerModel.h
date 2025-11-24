#ifndef DOIPSERVERMODEL_H
#define DOIPSERVERMODEL_H

#include <functional>

#include "DoIPAddress.h"
#include "DoIPEvent.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPState.h"

namespace doip {

// Forward declaration
class IConnectionContext;

using ServerModelCloseHandler = std::function<void(IConnectionContext&)>;
using ServerModelDiagnosticHandler = std::function<DoIPDiagnosticAck(IConnectionContext&, const DoIPMessage &)>;
using ServerModelDiagnosticNotificationHandler = std::function<void(IConnectionContext&, DoIPDiagnosticAck)>;

struct DoIPServerModel {

    ServerModelCloseHandler onCloseConnection;
    ServerModelDiagnosticHandler onDiagnosticMessage;
    ServerModelDiagnosticNotificationHandler onDiagnosticNotification;

    DoIPAddress serverAddress;
};

struct DefaultDoIPServerModel : public DoIPServerModel {
    DefaultDoIPServerModel() {
        onCloseConnection = [](IConnectionContext& ctx) noexcept {
            (void)ctx;
            // Default no-op
        };
        onDiagnosticMessage = [](IConnectionContext& ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            (void)msg;
            // Default: always ACK
            return std::nullopt;
        };
        onDiagnosticNotification = [](IConnectionContext& ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            (void)ack;
            // Default no-op
        };
    }
};

} // namespace doip

#endif /* DOIPSERVERMODEL_H */

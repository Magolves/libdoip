#ifndef DOIPSERVERMODEL_H
#define DOIPSERVERMODEL_H

#include <functional>

#include "DoIPAddress.h"
#include "DoIPCloseReason.h"
#include "DoIPDownstreamResult.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerEvent.h"
#include "DoIPServerState.h"
#include "Logger.h"

namespace doip {

// Forward declaration
class IConnectionContext;

// Callback type definitions
using ServerModelOpenHandler = std::function<void(IConnectionContext &)>;
using ServerModelCloseHandler = std::function<void(IConnectionContext &, DoIPCloseReason)>;
using ServerModelDiagnosticHandler = std::function<DoIPDiagnosticAck(IConnectionContext &, const DoIPMessage &)>;
using ServerModelDiagnosticNotificationHandler = std::function<void(IConnectionContext &, DoIPDiagnosticAck)>;

/**
 * @brief Callback for downstream (subnet) request handling
 *
 * This callback is invoked when a diagnostic message needs to be forwarded
 * to a downstream device (e.g., via CAN, LIN, or another transport).
 *
 * The implementation is responsible for:
 * 1. Sending the diagnostic message to the appropriate downstream device
 * 2. When a response is received, calling ctx.receiveDownstreamResponse(response)
 *
 * The state machine will handle timeout management internally.
 *
 * @param ctx The connection context (use for receiveDownstreamResponse callback)
 * @param msg The diagnostic message to forward downstream
 * @return DoIPDownstreamResult indicating the result of the request initiation
 *         - Pending: Async request initiated, response will come via receiveDownstreamResponse
 *         - Handled: Message was handled synchronously, no state transition needed
 *         - Error: Failed to initiate request, connection should handle error
 */
using ServerModelDownstreamHandler = std::function<DoIPDownstreamResult(IConnectionContext &ctx, const DoIPMessage &msg)>;

/**
 * @brief Callback for downstream response notification
 *
 * Optional callback invoked when a downstream response is received and
 * about to be sent back to the client. This allows the application to
 * inspect or log the response.
 *
 * @param ctx The connection context
 * @param request The original request that was sent downstream
 * @param response The response received from downstream
 */
using ServerModelDownstreamResponseHandler = std::function<void(IConnectionContext &ctx, const DoIPMessage &request, const DoIPMessage &response)>;

/**
 * @brief DoIP Server Model - Configuration and callbacks for a DoIP server connection
 *
 * This struct holds all application-specific configuration and callbacks
 * that customize the behavior of a DoIP server connection. It separates
 * the protocol logic (in DoIPServerStateMachine) from the application logic.
 */
struct DoIPServerModel {
    /// Called when the connection is being opened
    ServerModelOpenHandler onOpenConnection;

    /// Called when the connection is being closed
    ServerModelCloseHandler onCloseConnection;

    /// Called when a diagnostic message is received (for local handling)
    ServerModelDiagnosticHandler onDiagnosticMessage;

    /// Called after a diagnostic ACK/NACK was sent to the client
    ServerModelDiagnosticNotificationHandler onDiagnosticNotification;

    // === Downstream (subnet) callbacks ===

    /**
     * @brief Called when a diagnostic message should be forwarded downstream
     *
     * If this callback is set (not nullptr), the state machine will use it
     * to forward diagnostic messages to downstream devices. If not set,
     * messages are handled locally via onDiagnosticMessage.
     *
     * Typical usage:
     * - Set this for gateway ECUs that forward to CAN/LIN subnets
     * - Leave unset for end-node ECUs that handle messages locally
     */
    ServerModelDownstreamHandler onDownstreamRequest;

    /**
     * @brief Optional notification when downstream response is received
     *
     * Called before the response is sent back to the DoIP client.
     * Useful for logging, metrics, or response modification.
     */
    ServerModelDownstreamResponseHandler onDownstreamResponse;

    /// The logical address of this DoIP server
    DoIPAddress serverAddress = DoIPAddress(0x0E, 0x00);

    /**
     * @brief Check if downstream forwarding is enabled
     * @return true if onDownstreamRequest callback is set
     */
    bool hasDownstreamHandler() const { return onDownstreamRequest != nullptr; }
};

using UniqueServerModelPtr = std::unique_ptr<DoIPServerModel>;

/**
 * @brief Default DoIP Server Model with no-op callbacks
 *
 * This is used as a fallback when no model is configured. All callbacks
 * log warnings and perform no-op actions. Should only be used for testing
 * or as a placeholder during initialization.
 */
struct DefaultDoIPServerModel : public DoIPServerModel {
    DefaultDoIPServerModel() {
        onOpenConnection = [](IConnectionContext &ctx) noexcept {
            (void)ctx;
        };

        onCloseConnection = [](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
            (void)reason;
        };

        onDiagnosticMessage = [](IConnectionContext &ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            (void)msg;
            DOIP_LOG_DEBUG("Diagnostic message received on DefaultDoIPServerModel");
            // Default: always ACK
            return std::nullopt;
        };

        onDiagnosticNotification = [](IConnectionContext &ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            (void)ack;
            DOIP_LOG_DEBUG("Diagnostic notification on DefaultDoIPServerModel");
            // Default no-op
        };



        // Note: onDownstreamRequest is intentionally left as nullptr
        // This means downstream forwarding is disabled by default
        onDownstreamRequest = nullptr;
        onDownstreamResponse = nullptr;
    }

    ~DefaultDoIPServerModel()  {

    }

};

} // namespace doip

#endif /* DOIPSERVERMODEL_H */
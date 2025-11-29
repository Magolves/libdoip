
// DoIPServerModel.h
#pragma once

#include <memory>
#include <functional>
#include <optional>
#include "DoIPMessage.h"
#include "DoIPConnection.h"
#include "DoIPNegativeAck.h"

namespace doip {

// Forward declaration if needed
class DoIPConnection;

// Diagnostic message acknowledgment type (can be customized)
using DoIPDiagnosticAck = std::optional<DoIPNegativeAck>;

/**
 * @brief Abstract interface for DoIP server application logic.
 *
 * Users should derive from this class and override the relevant methods.
 * One instance is typically created per connection.
 */
class DoIPServerModel {
public:
    virtual ~DoIPServerModel() = default;

    /**
     * @brief Called when a new connection is established.
     * @param connection Pointer to the new connection.
     * @return true to accept, false to reject.
     */
    virtual bool onConnectionOpened(DoIPConnection* connection) { (void)connection; return true; }

    /**
     * @brief Called when a diagnostic message is received.
     * @param connection Pointer to the connection.
     * @param message The received diagnostic message.
     * @return Optional negative acknowledgment (std::nullopt for success).
     */
    virtual DoIPDiagnosticAck onDiagnosticMessage(DoIPConnection* connection, const DoIPMessage& message) {
        (void)connection; (void)message; return std::nullopt;
    }

    /**
     * @brief Called when the connection is about to close.
     * @param connection Pointer to the connection.
     */
    virtual void onConnectionClosed(DoIPConnection* connection) { (void)connection; }

    /**
     * @brief Called for any protocol or server error.
     * @param connection Pointer to the connection.
     * @param errorCode Error code or enum.
     * @param message Optional error message.
     */
    virtual void onError(DoIPConnection* connection, int errorCode, const std::string& message) {
        (void)connection; (void)errorCode; (void)message;
    }

    // Add more hooks as needed (e.g., for authentication, logging, etc.)
};

using DoIPServerModelPtr = std::shared_ptr<DoIPServerModel>;

} // namespace doip
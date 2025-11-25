#ifndef ICONNECTIONCONTEXT_H
#define ICONNECTIONCONTEXT_H

#include "DoIPAddress.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"
#include <cstdint>

namespace doip {

/**
 * @brief Reason for connection closure
 */
enum class CloseReason : uint8_t {
    None,
    InitialInactivityTimeout,
    GeneralInactivityTimeout,
    AliveCheckTimeout,
    SocketError,
    InvalidMessage,
    ApplicationRequest,
    RoutingActivationDenied
};

/**
 * @brief Interface between DoIPServerStateMachine and DoIPConnection
 *
 * This interface inverts the dependency between the state machine and connection.
 * Instead of the state machine holding callbacks to the connection, the connection
 * implements this interface and passes itself to the state machine.
 *
 * Benefits:
 * - No callback setup boilerplate
 * - Compile-time type checking
 * - Easier to test (mock the interface)
 * - Clear contract between layers
 *
 * Responsibilities:
 * - Bridge between protocol layer (state machine) and application layer (user callbacks)
 * - Provide connection-level operations (send, close)
 * - Provide configuration queries (addresses)
 */
class IConnectionContext {
public:
    virtual ~IConnectionContext() = default;

    /**
     * @brief Send a DoIP protocol message to the client
     *
     * This is called by the state machine when it needs to send a protocol
     * message (ACK, NACK, alive check, routing activation response, etc.)
     *
     * @param msg The DoIP message to send
     */
    [[nodiscard]] virtual ssize_t sendProtocolMessage(const DoIPMessage &msg) = 0;

    /**
     * @brief Close the TCP connection
     *
     * This is called by the state machine when the connection should be closed
     * (timeout, invalid message, etc.)
     *
     * @param reason Why the connection is being closed
     */
    virtual void closeConnection(CloseReason reason) = 0;

    /**
     * @brief Get the server's logical address
     *
     * Used by state machine for constructing response messages
     *
     * @return The server's DoIP logical address
     */
    virtual DoIPAddress getServerAddress() const = 0;

    /**
     * @brief Get the currently active source address (routed client)
     *
     * After routing activation, this returns the client's source address
     *
     * @return The active source address, or 0 if no routing is active
     */
    virtual uint16_t getActiveSourceAddress() const = 0;

    /**
     * @brief Set the active source address after routing activation
     *
     * Called by state machine when routing activation is successful
     *
     * @param address The client's source address
     */
    virtual void setActiveSourceAddress(uint16_t address) = 0;

    /**
     * @brief Handle an incoming diagnostic message (application callback)
     *
     * This forwards the diagnostic message to the application layer for processing.
     * The application returns either:
     * - std::nullopt: Send positive ACK
     * - DoIPNegativeDiagnosticAck value: Send negative ACK with this code
     *
     * @param msg The diagnostic message received
     * @return std::nullopt for ACK, or NACK code
     */
    virtual DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) = 0;

    /**
     * @brief Notify application that connection is closing
     *
     * This allows the application to perform cleanup when the connection
     * is closed (by timeout, error, or graceful shutdown)
     *
     * @param reason Why the connection is closing
     */
    virtual void notifyConnectionClosed(CloseReason reason) = 0;

    /**
     * @brief Notify application that diagnostic ACK/NACK was sent
     *
     * Optional notification after sending diagnostic message acknowledgement
     *
     * @param ack The ACK/NACK that was sent (nullopt = positive ACK)
     */
    virtual void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) = 0;
};

} // namespace doip

#endif /* ICONNECTIONCONTEXT_H */

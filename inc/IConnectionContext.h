#ifndef ICONNECTIONCONTEXT_H
#define ICONNECTIONCONTEXT_H

#include "DoIPAddress.h"
#include "DoIPCloseReason.h"
#include "DoIPDownstreamResult.h"
#include "DoIPMessage.h"
#include "DoIPNegativeDiagnosticAck.h"

#include <cstdint>

namespace doip {

/**
 * @brief Interface between DoIPServerStateMachine and DoIPConnection
 *
 * This interface inverts the dependency between the state machine and connection.
 * Instead of the state machine holding callbacks to the connection, the connection
 * implements this interface and passes itself to the state machine.
 *
 * Responsibilities:
 * - Bridge between protocol layer (state machine) and application layer (user callbacks)
 * - Provide connection-level operations (send, close)
 * - Provide configuration queries (addresses)
 * - Handle downstream (subnet) communication flow
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
     * @return Number of bytes sent, or negative value on error
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
    virtual void closeConnection(DoIPCloseReason reason) = 0;

    /**
     * @brief Check if the connection is currently open
     *
     * @return true if the connection is open, false otherwise
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief Get the reason why the connection was closed
     *
     * @return The DoIPCloseReason for the closed connection
     */
    virtual DoIPCloseReason getCloseReason() const = 0;

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
    virtual DoIPAddress getClientAddress() const = 0;

    /**
     * @brief Set the active source address after routing activation
     *
     * Called by state machine when routing activation is successful
     *
     * @param address The client's source address
     */
    virtual void setClientAddress(const DoIPAddress &address) = 0;

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
    virtual void notifyConnectionClosed(DoIPCloseReason reason) = 0;

    /**
     * @brief Notify application that diagnostic ACK/NACK was sent
     *
     * Optional notification after sending diagnostic message acknowledgement
     *
     * @param ack The ACK/NACK that was sent (nullopt = positive ACK)
     */
    virtual void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) = 0;

    /**
     * @brief Check if downstream forwarding is available
     *
     * This checks if the DoIPServerModel has a downstream handler configured.
     * The state machine uses this to decide whether to forward messages
     * downstream or handle them locally.
     *
     * @return true if downstream forwarding is configured, false otherwise
     */
    virtual bool hasDownstreamHandler() const = 0;

    /**
     * @brief Forward a diagnostic message to downstream (subnet device)
     *
     * Called by the state machine when a diagnostic message should be routed
     * to a downstream device (e.g., ECU on CAN bus). The implementation
     * delegates to DoIPServerModel::onDownstreamRequest.
     *
     * The downstream handler is responsible for:
     * 1. Sending the message via the appropriate transport (CAN, LIN, etc.)
     * 2. Storing the connection context for async response delivery
     * 3. Calling receiveDownstreamResponse() when the response arrives
     *
     * The state machine handles:
     * - Transition to WaitDownstreamResponse state
     * - Starting the downstream response timeout timer
     * - Processing the response when it arrives
     *
     * @param msg The diagnostic message to forward
     * @return Result indicating if the request was initiated successfully
     */
    virtual DoIPDownstreamResult notifyDownstreamRequest(const DoIPMessage &msg) = 0;

    /**
     * @brief Receive a response from downstream device
     *
     * Called by the application layer when a response is received from
     * a downstream device. This injects the response back into the
     * state machine for processing.
     *
     * Thread-safety: This method may be called from a different thread
     * than the one running the state machine. Implementations should
     * ensure thread-safe access to the state machine.
     *
     * Typical call sequence:
     * 1. State machine calls notifyDownstreamRequest(request)
     * 2. Application sends request via CAN/LIN/etc.
     * 3. Application receives response from downstream
     * 4. Application calls receiveDownstreamResponse(response)
     * 5. State machine processes response and sends to client
     *
     * @param response The diagnostic response from downstream
     */
    virtual void receiveDownstreamResponse(const ByteArray &response, DoIPDownstreamResult result) = 0;
};

} // namespace doip

#endif /* ICONNECTIONCONTEXT_H */
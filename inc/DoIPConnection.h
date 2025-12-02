#ifndef DOIPCONNECTION_H
#define DOIPCONNECTION_H


#include "DoIPMessage.h"
#include "DoIPNegativeAck.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerModel.h"
#include "DoIPDefaultConnection.h"
#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>


namespace doip {

/** Maximum size of the ISO-TP message - used as initial value for RX buffer to avoid reallocs */
constexpr size_t MAX_ISOTP_MTU = 4095;

class DoIPConnection : public DoIPDefaultConnection {
  public:

    DoIPConnection(int tcpSocket, UniqueServerModelPtr model);

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength);

    void sendDiagnosticPayload(const DoIPAddress &sourceAddress, const ByteArray &payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress &sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress &sourceAddress, DoIPNegativeDiagnosticAck ackCode);


    // === IConnectionContext interface implementation ===

    /**
     * @brief Send a DoIP protocol message to the client
     * @param msg The DoIP message to send
     */
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override;

    /**
     * @brief Close the TCP connection
     * @param reason Why the connection is being closed
     */
    void closeConnection(DoIPCloseReason reason) override;

    /**
     * @brief Get the server's logical address
     * @return The server's DoIP logical address
     */
    DoIPAddress getServerAddress() const override;

    /**
     * @brief Get the currently client (active source) address
     * @return The client (active source) address, or 0 if no routing is active
     */
    DoIPAddress getClientAddress() const override;

    /**
     * @brief Set the client (active source) address after routing activation
     * @param address The client's source address
     */
    void setClientAddress(const DoIPAddress& address) override;

    /**
     * @brief Handle an incoming diagnostic message (application callback)
     * @param msg The diagnostic message received
     * @return std::nullopt for ACK, or NACK code
     */
    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override;

    /**
     * @brief Notify application that connection is closing
     * @param reason Why the connection is closing
     */
    void notifyConnectionClosed(DoIPCloseReason reason) override;

    /**
     * @brief Notify application that diagnostic ACK/NACK was sent
     * @param ack The ACK/NACK that was sent
     */
    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override;

    // === Downstream (Subnet) Operations ===

    /**
     * @brief Check if downstream forwarding is available
     * @return true if the server model has a downstream handler configured
     */
    bool hasDownstreamHandler() const override;

    /**
     * @brief Forward a diagnostic message to downstream
     *
     * Delegates to DoIPServerModel::onDownstreamRequest callback.
     * The callback is responsible for sending the message via
     * the appropriate transport (CAN, LIN, etc.).
     *
     * @param msg The diagnostic message to forward
     * @return Result indicating if the request was initiated successfully
     */
    DoIPDownstreamResult notifyDownstreamRequest(const DoIPMessage &msg) override;

    /**
     * @brief Receive a response from downstream device
     *
     * Called by the application when a response arrives from downstream.
     * This injects the response into the state machine via
     * processEvent(DiagnosticMessageReceivedDownstream, response).
     *
     * @param response The diagnostic response from downstream
     */
    void receiveDownstreamResponse(const DoIPMessage &response) override;

    /**
     * @brief Notify application that downstream response was received
     *
     * Delegates to DoIPServerModel::onDownstreamResponse callback if set.
     *
     * @param request The original request that was sent downstream
     * @param response The response received from downstream
     */
    void notifyDownstreamResponseReceived(const DoIPMessage &request, const DoIPMessage &response) override;

  private:
    DoIPAddress m_gatewayAddress;

    // TCP socket-specific members
    int m_tcpSocket;
    std::array<uint8_t, MAX_ISOTP_MTU> m_receiveBuf{};
    bool m_isClosing{false};  // TODO: Guard against recursive closeConnection calls -> solve this
    std::optional<DoIPMessage> m_pendingDownstreamRequest;

    void closeSocket();

    int reactOnReceivedTcpMessage(const DoIPMessage &message);

    void handleMessage(const DoIPMessage &message);
    ssize_t sendMessage(const uint8_t *message, size_t messageLength);
};

} // namespace doip

#endif /* DOIPCONNECTION_H */
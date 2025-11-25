#ifndef DOIPCONNECTION_H
#define DOIPCONNECTION_H


#include "DoIPMessage.h"
#include "DoIPNegativeAck.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerModel.h"
#include "IConnectionContext.h"
#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "DoIPServerStateMachine.h"

namespace doip {

/** Maximum size of the ISO-TP message - used as initial value for RX buffer to avoid reallocs */
constexpr size_t MAX_ISOTP_MTU = 4095;

class DoIPConnection : public IConnectionContext {
  public:

    DoIPConnection(int tcpSocket, UniqueServerModelPtr model);

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength);

    void sendDiagnosticPayload(const DoIPAddress &sourceAddress, const ByteArray &payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress &sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress &sourceAddress, DoIPNegativeDiagnosticAck ackCode);


    /**
     * @brief Gets the server model configuration
     * @return Pointer to the current DoIPServerModel, or nullptr if not set
     */
    DoIPServerModel* getServerModel() const { return m_serverModel.get(); }

    /**
     * @brief Sets the server model configuration
     * @param model The DoIPServerModel to use for this connection
     */
    void setServerModel(UniqueServerModelPtr model) { m_serverModel = std::move(model); }

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
    void closeConnection(CloseReason reason) override;

    /**
     * @brief Get the server's logical address
     * @return The server's DoIP logical address
     */
    DoIPAddress getServerAddress() const override;

    /**
     * @brief Get the currently active source address
     * @return The active source address, or 0 if no routing is active
     */
    uint16_t getActiveSourceAddress() const override;

    /**
     * @brief Set the active source address after routing activation
     * @param address The client's source address
     */
    void setActiveSourceAddress(uint16_t address) override;

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
    void notifyConnectionClosed(CloseReason reason) override;

    /**
     * @brief Notify application that diagnostic ACK/NACK was sent
     * @param ack The ACK/NACK that was sent
     */
    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override;

  private:
    std::array<uint8_t, MAX_ISOTP_MTU> m_receiveBuf{};

    int m_tcpSocket;
    DoIPAddress m_gatewayAddress;
    DoIPAddress m_routedClientAddress;

    UniqueServerModelPtr m_serverModel;
    DoIPServerStateMachine m_stateMachine;

    bool m_isClosing{false};  // TODO: Guard against recursive closeConnection calls -> solve this

    void closeSocket();

    int reactOnReceivedTcpMessage(const DoIPMessage &message);

    void handleMessage(const DoIPMessage &message);
    ssize_t sendMessage(const uint8_t *message, size_t messageLength);
};

} // namespace doip

#endif /* DOIPCONNECTION_H */

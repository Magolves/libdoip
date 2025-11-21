#ifndef DOIPCONNECTION_H
#define DOIPCONNECTION_H


#include "DoIPMessage.h"
#include "DoIPNegativeAck.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerModel.h"
#include "DoIPServerStateMachine.h"
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

class DoIPConnection {
  public:

    DoIPConnection(int tcpSocket, const DoIPServerModel &model);

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength);

    void sendDiagnosticPayload(const DoIPAddress &sourceAddress, const ByteArray &payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress &sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress &sourceAddress, DoIPNegativeDiagnosticAck ackCode);


    /**
     * @brief Gets the server model configuration
     * @return Reference to the current DoIPServerModel
     */
    const DoIPServerModel& getServerModel() const { return m_serverModel; }

    /**
     * @brief Sets the server model configuration
     * @param model The DoIPServerModel to use for this connection
     */
    void setServerModel(const DoIPServerModel& model) { m_serverModel = model; }

  private:
    std::array<uint8_t, MAX_ISOTP_MTU> m_receiveBuf{};

    int m_tcpSocket;
    DoIPAddress m_gatewayAddress;
    DoIPAddress m_routedClientAddress;

    DoIPServerModel m_serverModel;
    DoIPServerStateMachine m_stateMachine;

    void closeSocket();

    int reactOnReceivedTcpMessage(const DoIPMessage &message);

    void handleMessage(const DoIPMessage &message);
    ssize_t sendMessage(const uint8_t *message, size_t messageLength);
};

} // namespace doip

#endif /* DOIPCONNECTION_H */

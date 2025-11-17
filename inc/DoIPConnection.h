#ifndef DOIPCONNECTION_H
#define DOIPCONNECTION_H

#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/if.h>
#include <unistd.h>
#include "DiagnosticMessageHandler.h"
#include "AliveCheckTimer.h"
#include "DoIPMessage.h"
#include "DoIPNegativeAck.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPServerStateMachine.h"

namespace doip {

const unsigned long _MaxDataSize = 0xFFFFFF;

/** Maximum size of the ISO-TP message - used as initial value for RX buffer to avoid reallocs */

class DoIPConnection {
    public:
    static constexpr size_t MAX_ISOTP_MTU = 4095;

    DoIPConnection(int tcpSocket, const DoIPAddress& gatewayAddress);

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength);

    void sendDiagnosticPayload(const DoIPAddress& sourceAddress, const ByteArray& payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress& sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress& sourceAddress, DoIPNegativeDiagnosticAck ackCode);
    int sendNegativeAck(DoIPNegativeAck ackCode);

    void setCallback(DiagnosticCallback dc, DiagnosticMessageNotification dmn, CloseConnectionHandler ccb);
    void setGeneralInactivityTime(const uint16_t seconds);

private:
    std::array<uint8_t, MAX_ISOTP_MTU> m_receiveBuf{};

    int m_tcpSocket;
    DoIPAddress m_gatewayAddress;
    DoIPServerStateMachine m_stateMachine;

    AliveCheckTimer m_aliveCheckTimer;
    DiagnosticCallback m_diag_callback;
    CloseConnectionHandler m_close_connection;
    DiagnosticMessageNotification m_notify_application;

    DoIPAddress m_routedClientAddress;

    void closeSocket();

    int reactOnReceivedTcpMessage(const DoIPMessage& message);

    ssize_t sendMessage(const uint8_t* message, size_t messageLength);

    void aliveCheckTimeout();
};

} // namespace doip

#endif /* DOIPCONNECTION_H */

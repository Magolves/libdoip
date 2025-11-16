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

using CloseConnectionCallback = std::function<void()>;

const unsigned long _MaxDataSize = 0xFFFFFF;

/** Maximum size of the ISO-TP message - used as initial value for RX buffer to avoid reallocs */
const size_t MAX_ISOTP_MTU = 4095;

class DoIPConnection {

public:
    DoIPConnection(int tcpSocket, const DoIPAddress& gatewayAddress):
        m_tcpSocket(tcpSocket), m_gatewayAddress(gatewayAddress) { };

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength);

    void sendDiagnosticPayload(const DoIPAddress& sourceAddress, const ByteArray& payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress& sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress& sourceAddress, DoIPNegativeDiagnosticAck ackCode);
    int sendNegativeAck(DoIPNegativeAck ackCode);

    void setCallback(DiagnosticCallback dc, DiagnosticMessageNotification dmn, CloseConnectionCallback ccb);
    void setGeneralInactivityTime(const uint16_t seconds);

private:
    std::array<uint8_t, MAX_ISOTP_MTU> m_receiveBuf{};

    int m_tcpSocket;
    DoIPServerStateMachine m_stateMachine;

    AliveCheckTimer m_aliveCheckTimer;
    DiagnosticCallback m_diag_callback;
    CloseConnectionCallback m_close_connection;
    DiagnosticMessageNotification m_notify_application;

    DoIPAddress m_routedClientAddress;
    DoIPAddress m_gatewayAddress;

    void closeSocket();

    int reactOnReceivedTcpMessage(const DoIPMessage& message);

    ssize_t sendMessage(const uint8_t* message, size_t messageLength);

    void aliveCheckTimeout();
};

} // namespace doip

#endif /* DOIPCONNECTION_H */

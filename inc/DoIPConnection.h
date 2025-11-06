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
#include "DoIPGenericHeaderHandler.h"
#include "RoutingActivationHandler.h"
#include "VehicleIdentificationHandler.h"
#include "DoIPGenericHeaderHandler.h"
#include "RoutingActivationHandler.h"
#include "DiagnosticMessageHandler.h"
#include "AliveCheckTimer.h"
#include "DoIPNegativeDiagnosticAck.h"

namespace doip {

using CloseConnectionCallback = std::function<void()>;

const unsigned long _MaxDataSize = 0xFFFFFF;

class DoIPConnection {

public:
    DoIPConnection(int tcpSocket, const DoIPAddress& logicalGatewayAddress):
        m_tcpSocket(tcpSocket), m_logicalGatewayAddress(logicalGatewayAddress) { };

    int receiveTcpMessage();
    size_t receiveFixedNumberOfBytesFromTCP(size_t payloadLength, uint8_t *receivedData);

    void sendDiagnosticPayload(const DoIPAddress& sourceAddress, const ByteArray& payload);
    bool isSocketActive() { return m_tcpSocket != 0; };

    void triggerDisconnection();

    void sendDiagnosticAck(const DoIPAddress& sourceAddress);
    void sendDiagnosticNegativeAck(const DoIPAddress& sourceAddress, DoIPNegativeDiagnosticAck ackCode);
    int sendNegativeAck(uint8_t ackCode);

    void setCallback(DiagnosticCallback dc, DiagnosticMessageNotification dmn, CloseConnectionCallback ccb);
    void setGeneralInactivityTime(const uint16_t seconds);

private:

    int m_tcpSocket;

    AliveCheckTimer m_aliveCheckTimer;
    DiagnosticCallback m_diag_callback;
    CloseConnectionCallback m_close_connection;
    DiagnosticMessageNotification m_notify_application;

    DoIPAddress m_routedClientAddress;
    DoIPAddress m_logicalGatewayAddress;

    void closeSocket();

    int reactOnReceivedTcpMessage(GenericHeaderAction action, size_t payloadLength, uint8_t *payload);

    ssize_t sendMessage(uint8_t* message, size_t messageLength);
    ssize_t sendMessage(const ByteArray& message);

    void aliveCheckTimeout();
};

} // namespace doip

#endif /* DOIPCONNECTION_H */

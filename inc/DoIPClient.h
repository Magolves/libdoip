
#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H

#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "DoIPConfig.h"
#include "DoIPMessage.h"

namespace doip {

const int _maxDataSize = 64;

using DoIPRequest = std::pair<size_t, const uint8_t *>;

class DoIPClient {

  public:
    DoIPClient() {m_receiveBuf.reserve(DOIP_MAXIMUM_MTU);}

    void startTcpConnection();
    void startUdpConnection();
    void startAnnouncementListener();
    ssize_t sendRoutingActivationRequest();
    ssize_t sendVehicleIdentificationRequest(const char *inet_address);
    void receiveRoutingActivationResponse();
    void receiveUdpMessage();
    [[nodiscard]]
    bool receiveVehicleAnnouncement();
    /*
     * Send the builded request over the tcp-connection to server
     */
    void receiveMessage();

    /**
     * Sends a diagnostic message to the server
     * @param payload          data that will be given to the ecu
     */
    ssize_t sendDiagnosticMessage(const ByteArray &payload);

    /**
     * Sends a alive check response containing the clients source address to the server
     */
    ssize_t sendAliveCheckResponse();
    void setSourceAddress(const DoIPAddress &address);
    void printVehicleInformationResponse();
    void closeTcpConnection();
    void closeUdpConnection();
    void reconnectServer();

    int getSockFd();
    int getConnected();

  private:
    ByteArray m_receiveBuf;
    int m_tcpSocket{-1}, m_udpSocket{-1}, m_udpAnnouncementSocket{-1}, m_connected{-1};
    int m_broadcast = 1;
    struct sockaddr_in m_serverAddress, m_clientAddress, m_announcementAddress;
    DoIPAddress m_sourceAddress = DoIPAddress(0xE000);

    DoIpVin m_vin{0};
    DoIPAddress m_logicalAddress = ZERO_ADDRESS;
    DoIpEid m_eid{0};
    DoIpGid m_gid{0};
    DoIPFurtherAction m_furtherActionReqResult = DoIPFurtherAction::NoFurtherAction;

    void parseVehicleIdentificationResponse(const DoIPMessage& msg);

    int emptyMessageCounter = 0;
};

} // namespace doip

#endif /* DOIPCLIENT_H */

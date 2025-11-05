
#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H
#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "DiagnosticMessageHandler.h"
#include "DoIPGenericHeaderHandler.h"

namespace doip {

const int _serverPortNr = 13400;
const int _maxDataSize = 64;

using DoIPRequest = std::pair<size_t, const uint8_t *>;

class DoIPClient {

  public:
    void startTcpConnection();
    void startUdpConnection();
    ssize_t sendRoutingActivationRequest();
    ssize_t sendVehicleIdentificationRequest(const char *inet_address);
    void receiveRoutingActivationResponse();
    void receiveUdpMessage();
    void receiveMessage();
    ssize_t sendDiagnosticMessage(const DoIPAddress &targetAddress, const ByteArray& payload);
    ssize_t sendAliveCheckResponse();
    void setSourceAddress(const DoIPAddress &address);
    void displayVIResponseInformation();
    void closeTcpConnection();
    void closeUdpConnection();
    void reconnectServer();

    int getSockFd();
    int getConnected();

  private:
    uint8_t _receivedData[_maxDataSize];
    int _sockFd, _sockFd_udp, _connected;
    int broadcast = 1;
    struct sockaddr_in _serverAddr, _clientAddr;
    DoIPAddress m_sourceAddress;

    uint8_t VINResult[17];
    DoIPAddress LogicalAddressResult;
    uint8_t EIDResult[6];
    uint8_t GIDResult[6];
    uint8_t FurtherActionReqResult;

    const DoIPRequest buildRoutingActivationRequest();
    const DoIPRequest buildVehicleIdentificationRequest();
    void parseVIResponseInformation(uint8_t *data);

    int emptyMessageCounter = 0;
};

} // namespace doip

#endif /* DOIPCLIENT_H */

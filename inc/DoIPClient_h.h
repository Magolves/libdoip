
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
    /*
     * Send the builded request over the tcp-connection to server
     */
    void receiveMessage();

    /**
     * Sends a diagnostic message to the server
     * @param targetAddress     the address of the ecu which should receive the message
     * @param payload          data that will be given to the ecu
     */
    ssize_t sendDiagnosticMessage(const DoIPAddress &targetAddress, const ByteArray &payload);

    /**
     * Sends a alive check response containing the clients source address to the server
     */
    ssize_t sendAliveCheckResponse();
    void setSourceAddress(const DoIPAddress &address);
    void displayVIResponseInformation();
    void closeTcpConnection();
    void closeUdpConnection();
    void reconnectServer();

    int getSockFd();
    int getConnected();

  private:
    uint8_t _receivedData[_maxDataSize] = {0};
    int _sockFd{-1}, _sockFd_udp{-1}, _connected{-1};
    int broadcast = 1;
    struct sockaddr_in _serverAddr, _clientAddr;
    DoIPAddress m_sourceAddress = ZeroAddress;

    uint8_t VINResult[17] = {0};
    DoIPAddress LogicalAddressResult = ZeroAddress;
    uint8_t EIDResult[6] = {0};
    uint8_t GIDResult[6] = {0};
    uint8_t FurtherActionReqResult = 0x00;

    /*
     *Build the Routing-Activation-Request for server
     */
    const DoIPRequest buildRoutingActivationRequest();
    const DoIPRequest buildVehicleIdentificationRequest();
    void parseVIResponseInformation(const uint8_t *data);

    int emptyMessageCounter = 0;
};

} // namespace doip

#endif /* DOIPCLIENT_H */


#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstddef>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>

#include "DiagnosticMessageHandler.h"
#include "DoIPGenericHeaderHandler.h"

const int _serverPortNr=13400;
const int _maxDataSize=64;


class DoIPClient{

public:
    void startTcpConnection();
    void startUdpConnection();
    void sendRoutingActivationRequest();
    void sendVehicleIdentificationRequest(const char* address);
    void receiveRoutingActivationResponse();
    void receiveUdpMessage();
    void receiveMessage();
    void sendDiagnosticMessage(uint8_t* targetAddress, uint8_t* userData, int userDataLength);
    void sendAliveCheckResponse();
    void setSourceAddress(uint8_t* address);
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
    uint8_t sourceAddress [2];

    uint8_t VINResult [17];
    uint8_t LogicalAddressResult [2];
    uint8_t EIDResult [6];
    uint8_t GIDResult [6];
    uint8_t FurtherActionReqResult;

    const std::pair<int, uint8_t*>* buildRoutingActivationRequest();
    const std::pair<int, uint8_t*>* buildVehicleIdentificationRequest();
    void parseVIResponseInformation(uint8_t* data);

    int emptyMessageCounter = 0;
};



#endif /* DOIPCLIENT_H */


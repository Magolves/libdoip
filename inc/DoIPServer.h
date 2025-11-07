#ifndef DOIPSERVER_H
#define DOIPSERVER_H

#include <iostream>
#include <optional>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/if.h>
#include <unistd.h>
#include <memory>
#include "RoutingActivationHandler.h"
#include "VehicleIdentificationHandler.h"

#include "RoutingActivationHandler.h"
#include "DiagnosticMessageHandler.h"
#include "AliveCheckTimer.h"
#include "DoIPConnection.h"

namespace doip {

using CloseConnectionCallback = std::function<void()>;

const int _ServerPort = 13400;
//const unsigned long _MaxDataSize = 4294967294;
//const unsigned long _MaxDataSize = 0xFFFFFF;

class DoIPServer {

public:
    DoIPServer() = default;

    void setupTcpSocket();
    std::unique_ptr<DoIPConnection> waitForTcpConnection();
    void setupUdpSocket();
    int receiveUdpMessage();

    void closeTcpSocket();
    void closeUdpSocket();

    int sendVehicleAnnouncement();

    void setEIDdefault();
    void setVIN(const std::string& VINString);
    void setLogicalGatewayAddress(const unsigned short inputLogAdd);
    void setEID(const uint64_t inputEID);
    void setGID(const uint64_t inputGID);
    void setFAR(const unsigned int inputFAR);
    void setA_DoIP_Announce_Num(int Num);
    void setA_DoIP_Announce_Interval(unsigned int Interval);

private:

    int server_socket_tcp{-1}, server_socket_udp{-1};
    struct sockaddr_in serverAddress, clientAddress;
    uint8_t data[_MaxDataSize] = {0};

    std::string VIN = "00000000000000000";
    DoIPAddress logicalGatewayAddress = ZeroAddress;
    uint8_t EID [6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // entity id
    uint8_t GID [6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // group id
    uint8_t FurtherActionReq = 0x00;

    int A_DoIP_Announce_Num = 3;    //Default Value = 3
    unsigned int A_DoIP_Announce_Interval = 500; //Default Value = 500ms

    int broadcast = 1;

    int reactToReceivedUdpMessage(size_t bytesRead);

    int sendUdpMessage(uint8_t* message, size_t messageLength);

    void setMulticastGroup(const char* address);
};

} // namespace doip

#endif /* DOIPSERVER_H */
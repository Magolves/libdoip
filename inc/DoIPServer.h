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

#include "RoutingActivationHandler.h"
#include "DiagnosticMessageHandler.h"
#include "AliveCheckTimer.h"
#include "DoIPConnection.h"
#include "DoIPFurtherAction.h"
#include "MacAddress.h"

namespace doip {

using CloseConnectionCallback = std::function<void()>;

const int DOIP_SERVER_PORT = 13400;


/**
 * @brief DoIP Server class to handle incoming DoIP connections and UDP messages.
 *
 */
class DoIPServer {

public:
    DoIPServer() {
        m_receiveBuf.reserve(MAX_ISOTP_MTU);
    };

    void setupTcpSocket();
    std::unique_ptr<DoIPConnection> waitForTcpConnection();
    void setupUdpSocket();
    ssize_t receiveUdpMessage();

    void setAnnounceNum(int Num);
    void setAnnounceInterval(unsigned int Interval);

    void closeTcpSocket();
    void closeUdpSocket();

    void setLogicalGatewayAddress(const unsigned short inputLogAdd);

    ssize_t sendVehicleAnnouncement();

    /**
     * @brief Sets the EID to a default value based on the MAC address.
     *
     * @return true if the EID was successfully set to the default value.
     * @return false if the default EID could not be set.
     */
    bool setEIDdefault();
    
    void setVIN(const std::string& VINString);
    const DoIPVIN& getVIN() const { return m_VIN; }

    void setEID(const uint64_t inputEID);
    const DoIPEID& getEID() const { return m_EID; }

    void setGID(const uint64_t inputGID);
    const DoIPGID& getGID() const { return m_GID; }
    void setFAR(DoIPFurtherAction inputFAR);



private:

    int m_tcp_sock{-1};
    int m_udp_sock{-1};
    struct sockaddr_in m_serverAddress{};
    struct sockaddr_in m_clientAddress{};
    ByteArray m_receiveBuf{};

    DoIPVIN m_VIN;
    DoIPAddress m_gatewayAddress = DoIPAddress::ZeroAddress;
    DoIPEID m_EID = DoIPEID::Zero;
    DoIPGID m_GID = DoIPGID::Zero;
    DoIPFurtherAction m_FurtherActionReq = DoIPFurtherAction::NoFurtherAction;

    int m_announceNum = 3;    //Default Value = 3
    unsigned int m_announceInterval = 500; //Default Value = 500ms

    int m_broadcast = 1;

    ssize_t reactToReceivedUdpMessage(size_t bytesRead);
    ssize_t sendUdpMessage(const uint8_t* message, size_t messageLength);

    void setMulticastGroup(const char* address);

    ssize_t sendNegativeUdpAck(DoIPNegativeAck ackCode);

};

} // namespace doip

#endif /* DOIPSERVER_H */
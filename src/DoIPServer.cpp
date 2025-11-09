#include "DoIPServer.h"
#include "DoIPMessage.h"
#include "MacAddress.h"

using namespace doip;

#if defined(__linux__)
const char* DEFAULT_IFACE = "eth0";
#elif defined(__APPLE__)
const char* DEFAULT_IFACE = "en0";
#else
#pragmea error "Unsupported platform"
#endif


/*
 * Set up a tcp socket, so the socket is ready to accept a connection
 */
void DoIPServer::setupTcpSocket() {

    m_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_PORT);

    // binds the socket to the address and port number
    bind(m_tcp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
}

/*
 *  Wait till a client attempts a connection and accepts it
 */
std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection() {
    // waits till client approach to make connection
    listen(m_tcp_sock, 5);
    int tcpSocket = accept(m_tcp_sock, nullptr, nullptr);
    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, m_gatewayAddress));
}

void DoIPServer::setupUdpSocket() {

    m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_PORT);

    if (m_udp_sock < 0)
        std::cout << "Error setting up a udp socket" << '\n';

    // binds the socket to any IP DoIPAddress and the Port Number 13400
    bind(m_udp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));

    // setting the IP DoIPAddress for Multicast
    setMulticastGroup("224.0.0.2");
}

/*
 * Closes the socket for this server
 */
void DoIPServer::closeTcpSocket() {
    close(m_tcp_sock);
}

void DoIPServer::closeUdpSocket() {
    close(m_udp_sock);
}

/*
 * Receives a udp message and calls reactToReceivedUdpMessage method
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
ssize_t DoIPServer::receiveUdpMessage() {

    unsigned int length = sizeof(m_serverAddress);
    const ssize_t readBytes = recvfrom(m_udp_sock, m_receiveBuf.data(), m_receiveBuf.size(), 0, reinterpret_cast<struct sockaddr *>(&m_serverAddress), &length);

    if (readBytes < 0) {
        std::cerr << "Error receiving UDP message" << '\n';
        return -1;
    }

    ssize_t sentBytes = reactToReceivedUdpMessage(static_cast<size_t>(readBytes));

    return sentBytes;
}

/*
 * Receives a udp message and determine how to process the message
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
ssize_t DoIPServer::reactToReceivedUdpMessage(size_t bytesRead) {
    (void)bytesRead;
    ssize_t sentBytes = -1;
    // GenericHeaderAction action = parseGenericHeader(data, bytesRead);

    auto optHeader = DoIPMessage::tryParseHeader(m_receiveBuf.data(), DOIP_HEADER_SIZE);
    if (!optHeader.has_value()) {
        return sendNegativeUdpAck(DoIPNegativeAck::IncorrectPatternFormat);
    }

    auto plType = optHeader->first;
    //auto payloadLength = optHeader->second;
    std::cout << "Received UDP message with Payload Type: " << plType << '\n';
    switch (plType) {
    case DoIPPayloadType::VehicleIdentificationRequest: {
        DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID);
        std::cout << "Send " << msg << '\n';
        ssize_t sendedBytes = sendUdpMessage(msg.data(), DOIP_HEADER_SIZE + msg.size());

        return static_cast<int>(sendedBytes);
    }

    default: {
        std::cerr << "not handled payload type occured in receiveUdpMessage()" << '\n';
            return sendNegativeUdpAck(DoIPNegativeAck::UnknownPayloadType);
    }
    }
    return sentBytes;
}

ssize_t DoIPServer::sendUdpMessage(const uint8_t *message, size_t messageLength) { // sendUdpMessage after receiving a message from the client
    // if the server receives a message from a client, than the response should be send back to the client address and port
    m_clientAddress.sin_port = m_serverAddress.sin_port;
    m_clientAddress.sin_addr.s_addr = m_serverAddress.sin_addr.s_addr;

    int result = sendto(m_udp_sock, message, messageLength, 0, reinterpret_cast<const struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));
    return result;
}


bool DoIPServer::setEIDdefault() {
    MacAddress mac = {0};
    if (!getFirstMacAddress(mac)) {
        std::cerr << "Failed to get MAC address, using default EID" << '\n';
        m_EID = DoIPEID::Zero;
        return false;
    }
    // Set EID based on MAC address (last 6 bytes)
    m_EID = DoIPEID(mac.data(), m_EID.ID_LENGTH);
    return true;
}

void DoIPServer::setVIN(const std::string &VINString) {

    m_VIN = DoIPVIN(VINString);
}

void DoIPServer::setLogicalGatewayAddress(const unsigned short inputLogAdd) {
    m_gatewayAddress.update(inputLogAdd);
}

void DoIPServer::setEID(const uint64_t inputEID) {
    m_EID = DoIPEID(inputEID);
}

void DoIPServer::setGID(const uint64_t inputGID) {
    m_GID = DoIPGID(inputGID);
}

void DoIPServer::setFAR(DoIPFurtherAction inputFAR) {
    m_FurtherActionReq = inputFAR;
}

void DoIPServer::setAnnounceNum(int Num) {
    m_announceNum = Num;
}

void DoIPServer::setAnnounceInterval(unsigned int Interval) {
    m_announceInterval = Interval;
}

void DoIPServer::setMulticastGroup(const char *address) {
    int loop = 1;

    // set Option using the same Port for multiple Sockets
    int setPort = setsockopt(m_udp_sock, SOL_SOCKET, SO_REUSEADDR, &loop, sizeof(loop));

    if (setPort < 0) {
        std::cout << "Setting Port Error" << '\n';
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    // set Option to join Multicast Group
    int setGroup = setsockopt(m_udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mreq), sizeof(mreq));

    if (setGroup < 0) {
        std::cout << "Setting DoIPAddress Error" << '\n';
    }
}

ssize_t DoIPServer::sendVehicleAnnouncement() {

    const char *address = "255.255.255.255";

    // setting the destination port for the Announcement to 13401
    m_clientAddress.sin_port = htons(13401);

    int setAddressError = inet_aton(address, &(m_clientAddress.sin_addr));

    if (setAddressError != 0) {
        std::cout << "Broadcast DoIPAddress set successfully" << '\n';
    }

    int socketError = setsockopt(m_udp_sock, SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));

    if (socketError == 0) {
        std::cout << "Broadcast Option set successfully" << '\n';
    }

    ssize_t sentBytes = -1;

    // uint8_t *message = createVehicleIdentificationResponse(VIN, m_gatewayAddress, m_EID, GID, FurtherActionReq);
    DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID, m_FurtherActionReq);

    for (int i = 0; i < m_announceNum; i++) {
        sentBytes = sendto(m_udp_sock, msg.data(), msg.size(), 0, reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));

        if (sentBytes > 0) {
            std::cout << "Sending Vehicle Announcement" <<  '\n';
            std::cout << msg << '\n';
        } else {
            std::cerr << "Failed Sending Vehicle Announcement" << msg << '\n';
        }
        usleep(m_announceInterval * 1000);
    }
    return sentBytes;
}

ssize_t DoIPServer::sendNegativeUdpAck(DoIPNegativeAck ackCode) {
    DoIPMessage message = message::makeNegativeAckMessage(ackCode);
    return sendUdpMessage(message.data(), message.size());
}

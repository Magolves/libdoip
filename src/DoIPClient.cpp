#include "DoIPClient.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"
#include <cerrno>  // for errno
#include <cstring> // for strerror

using namespace doip;

/*
 *Set up the connection between client and server
 */
void DoIPClient::startTcpConnection() {

    m_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_tcpSocket >= 0) {
        LOG_TCP_INFO("Client TCP-Socket created successfully");

        bool connectedFlag = false;
        const char *ipAddr = "127.0.0.1";
        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
        inet_aton(ipAddr, &(m_serverAddress.sin_addr));

        while (!connectedFlag) {
            m_connected = connect(m_tcpSocket, reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
            if (m_connected != -1) {
                connectedFlag = true;
                LOG_TCP_INFO("Connection to server established");
            }
        }
    }
}

void DoIPClient::startUdpConnection() {

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (m_udpSocket >= 0) {
        LOG_UDP_INFO("Client-UDP-Socket created successfully");

        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT); // 13400
        m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        m_clientAddress.sin_family = AF_INET;
        m_clientAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
        m_clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        // binds the socket to any IP DoIPAddress and the Port Number 13400
        bind(m_udpSocket, reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));
    }
}

void DoIPClient::startAnnouncementListener() {
    m_udpAnnouncementSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (m_udpAnnouncementSocket >= 0) {
        LOG_UDP_INFO("Client-Announcement-Socket created successfully");

        // Allow socket reuse for broadcast
        int reuse = 1;
        setsockopt(m_udpAnnouncementSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Enable broadcast reception
        int broadcast = 1;
        if (setsockopt(m_udpAnnouncementSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            LOG_UDP_ERROR("Failed to enable broadcast reception: {}", strerror(errno));
        } else {
            LOG_UDP_INFO("Broadcast reception enabled for announcements");
        }

        m_announcementAddress.sin_family = AF_INET;
        m_announcementAddress.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT); // Port 13401
        m_announcementAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind to port 13401 for Vehicle Announcements
        if (bind(m_udpAnnouncementSocket, reinterpret_cast<struct sockaddr *>(&m_announcementAddress), sizeof(m_announcementAddress)) < 0) {
            LOG_UDP_ERROR("Failed to bind announcement socket to port {}: {}", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT, strerror(errno));
        } else {
            LOG_UDP_INFO("Announcement socket bound to port {} successfully", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
        }
    } else {
        LOG_UDP_ERROR("Failed to create announcement socket: {}", strerror(errno));
    }
}

/*
 * closes the client-socket
 */
void DoIPClient::closeTcpConnection() {
    close(m_tcpSocket);
}

void DoIPClient::closeUdpConnection() {
    close(m_udpSocket);
    if (m_udpAnnouncementSocket >= 0) {
        close(m_udpAnnouncementSocket);
    }
}

void DoIPClient::reconnectServer() {
    closeTcpConnection();
    startTcpConnection();
}

ssize_t DoIPClient::sendRoutingActivationRequest() {
    DoIPMessage routingActReq = message::makeRoutingActivationRequest(m_sourceAddress);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(routingActReq));
    return write(m_tcpSocket, routingActReq.data(), routingActReq.size());
}

ssize_t DoIPClient::sendDiagnosticMessage(const ByteArray &payload) {
    DoIPMessage msg = message::makeDiagnosticMessage(m_sourceAddress, m_logicalAddress, payload);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(msg));

    return write(m_tcpSocket, msg.data(), msg.size());
}

ssize_t DoIPClient::sendAliveCheckResponse() {
    DoIPMessage msg = message::makeAliveCheckResponse(m_sourceAddress);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(msg));
    return write(m_tcpSocket, msg.data(), msg.size());
}

/*
 * Receive a message from server
 */
void DoIPClient::receiveMessage() {

    ssize_t bytesRead = recv(m_tcpSocket, m_receiveBuf.data(), _maxDataSize, 0);

    if (bytesRead < 0) {
        LOG_DOIP_ERROR("Error receiving data from server");
        return;
    }

    if (!bytesRead) // if server is disconnected from client; client gets empty messages
    {
        emptyMessageCounter++;

        if (emptyMessageCounter == 5) {
            LOG_DOIP_WARN("Received too many empty messages. Reconnect TCP connection");
            emptyMessageCounter = 0;
            reconnectServer();
        }
        return;
    }

    auto optMmsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        LOG_DOIP_ERROR("Failed to parse DoIP message from received data");
        return;
    }
    DoIPMessage msg = optMmsg.value();
    LOG_TCP_INFO("RX: {}", fmt::streamed(msg));
}

void DoIPClient::receiveUdpMessage() {

    unsigned int length = sizeof(m_clientAddress);

    // Set socket to timeout after 3 seconds
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int bytesRead;
    bytesRead = recvfrom(m_udpSocket, m_receiveBuf.data(), _maxDataSize, 0, reinterpret_cast<struct sockaddr *>(&m_clientAddress), &length);

    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            LOG_UDP_WARN("Timeout waiting for UDP response");
        } else {
            LOG_UDP_ERROR("Error receiving UDP message: {}", strerror(errno));
        }
        return;
    }

    LOG_UDP_INFO("Received {} bytes from UDP", bytesRead);

    auto optMmsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        LOG_UDP_ERROR("Failed to parse DoIP message from UDP data");
        return;
    }

    DoIPMessage msg = optMmsg.value();

    LOG_UDP_INFO("RX: {}", fmt::streamed(msg));
}

bool DoIPClient::receiveVehicleAnnouncement() {
    unsigned int length = sizeof(m_announcementAddress);
    int bytesRead;

    LOG_UDP_DEBUG("Listening for Vehicle Announcements on port {}", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

    // Set socket to non-blocking mode for timeout
    struct timeval timeout;
    timeout.tv_sec = 2; // 2 second timeout
    timeout.tv_usec = 0;
    setsockopt(m_udpAnnouncementSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    bytesRead = recvfrom(m_udpAnnouncementSocket, m_receiveBuf.data(), _maxDataSize, 0,
                         reinterpret_cast<struct sockaddr *>(&m_announcementAddress), &length);
    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            LOG_UDP_WARN("Timeout waiting for Vehicle Announcement");
        } else {
            LOG_UDP_ERROR("Error receiving Vehicle Announcement: {}", strerror(errno));
        }
        return false;
    }

    auto optMsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMsg.has_value()) {
        LOG_UDP_ERROR("Failed to parse Vehicle Announcement message");
        return false;
    }

    DoIPMessage msg = optMsg.value();
    // Parse and display the announcement information
    if (msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse) {
        LOG_UDP_INFO("Vehicle Announcement received: {}", fmt::streamed(msg));
        parseVehicleIdentificationResponse(msg);
        return true;
    }
    return false;
}

ssize_t DoIPClient::sendVehicleIdentificationRequest(const char *inet_address) {

    int setAddressError = inet_aton(inet_address, &(m_serverAddress.sin_addr));

    if (setAddressError != 0) {
        LOG_UDP_INFO("Address set successfully");
    } else {
        LOG_UDP_ERROR("Could not set address. Try again");
    }

    int socketError = setsockopt(m_udpSocket, SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));

    if (socketError == 0) {
        LOG_UDP_INFO("Broadcast Option set successfully");
    }

    DoIPMessage vehicleIdReq = message::makeVehicleIdentificationRequest();

    ssize_t bytesSent = sendto(m_udpSocket, vehicleIdReq.data(), vehicleIdReq.size(), 0, reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
    LOG_UDP_INFO("Sent Vehicle Identification Request to {}:{}", inet_address, ntohs(m_serverAddress.sin_port));

    if (bytesSent > 0) {
        LOG_DOIP_INFO("Sending Vehicle Identification Request");
    }

    return bytesSent;
}

/**
 * Sets the source address for this client
 * @param address   source address for the client
 */
void DoIPClient::setSourceAddress(const DoIPAddress &address) {
    m_sourceAddress = address;
}

/*
 * Getter for _sockFD
 */
int DoIPClient::getSockFd() {
    return m_tcpSocket;
}

/*
 * Getter for m_connected
 */
int DoIPClient::getConnected() {
    return m_connected;
}

void DoIPClient::parseVehicleIdentificationResponse(const DoIPMessage &msg) {
    auto optVin = msg.getVin();
    auto optEid = msg.getEid();
    auto optGid = msg.getGid();
    auto optLogicalAddress = msg.getLogicalAddress();
    auto optFurtherAction = msg.getFurtherActionRequest();

    if (!optVin || !optEid || !optGid || !optLogicalAddress || !optFurtherAction) {
        LOG_DOIP_WARN("Incomplete Vehicle Identification Response received: Missing VIN, EID, GID, Logical Address or Further Action Request");
    }

    m_vin = optVin.value();
    m_eid = optEid.value();
    m_gid = optGid.value();
    m_logicalAddress = optLogicalAddress.value();
    m_furtherActionReqResult = optFurtherAction.value();
}

void DoIPClient::printVehicleInformationResponse() {
    std::ostringstream ss;
    // output VIN
    ss << "VIN: "   ;
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    ss << m_vin << ansi::reset  ;
    LOG_DOIP_INFO(ss.str());

    // output LogicalAddress
    ss = std::ostringstream{};
    ss << "LA : ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    ss << m_logicalAddress << ansi::reset;
    LOG_DOIP_INFO(ss.str());

    // output EID
    ss = std::ostringstream{};
    ss << "EID: ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    ss << m_eid << ansi::reset;
    LOG_DOIP_INFO(ss.str());

    // output GID
    ss = std::ostringstream{};
    ss << "GID: ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    ss << m_gid << ansi::reset;
    LOG_DOIP_INFO(ss.str());

    // output FurtherActionRequest
    ss = std::ostringstream{};
    ss << "FAR: ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    ss << m_furtherActionReqResult << ansi::reset;
    LOG_DOIP_INFO(ss.str());
}

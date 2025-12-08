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

    _sockFd = socket(AF_INET, SOCK_STREAM, 0);

    if (_sockFd >= 0) {
        LOG_TCP_INFO("Client TCP-Socket created successfully");

        bool connectedFlag = false;
        const char *ipAddr = "127.0.0.1";
        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
        inet_aton(ipAddr, &(_serverAddr.sin_addr));

        while (!connectedFlag) {
            _connected = connect(_sockFd, reinterpret_cast<struct sockaddr *>(&_serverAddr), sizeof(_serverAddr));
            if (_connected != -1) {
                connectedFlag = true;
                LOG_TCP_INFO("Connection to server established");
            }
        }
    }
}

void DoIPClient::startUdpConnection() {

    _sockFd_udp = socket(AF_INET, SOCK_DGRAM, 0);

    if (_sockFd_udp >= 0) {
        LOG_UDP_INFO("Client-UDP-Socket created successfully");

        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT); // 13400
        _serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        _clientAddr.sin_family = AF_INET;
        _clientAddr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
        _clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // binds the socket to any IP DoIPAddress and the Port Number 13400
        bind(_sockFd_udp, reinterpret_cast<struct sockaddr *>(&_clientAddr), sizeof(_clientAddr));
    }
}

void DoIPClient::startAnnouncementListener() {
    _sockFd_announcement = socket(AF_INET, SOCK_DGRAM, 0);

    if (_sockFd_announcement >= 0) {
        LOG_UDP_INFO("Client-Announcement-Socket created successfully");

        // Allow socket reuse for broadcast
        int reuse = 1;
        setsockopt(_sockFd_announcement, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Enable broadcast reception
        int broadcast = 1;
        if (setsockopt(_sockFd_announcement, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            LOG_UDP_ERROR("Failed to enable broadcast reception: {}", strerror(errno));
        } else {
            LOG_UDP_INFO("Broadcast reception enabled for announcements");
        }

        _announcementAddr.sin_family = AF_INET;
        _announcementAddr.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT); // Port 13401
        _announcementAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind to port 13401 for Vehicle Announcements
        if (bind(_sockFd_announcement, reinterpret_cast<struct sockaddr *>(&_announcementAddr), sizeof(_announcementAddr)) < 0) {
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
    close(_sockFd);
}

void DoIPClient::closeUdpConnection() {
    close(_sockFd_udp);
    if (_sockFd_announcement >= 0) {
        close(_sockFd_announcement);
    }
}

void DoIPClient::reconnectServer() {
    closeTcpConnection();
    startTcpConnection();
}

ssize_t DoIPClient::sendRoutingActivationRequest() {
    DoIPMessage routingActReq = message::makeRoutingActivationRequest(m_sourceAddress);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(routingActReq));
    return write(_sockFd, routingActReq.data(), routingActReq.size());
}

ssize_t DoIPClient::sendDiagnosticMessage(const ByteArray &payload) {
    DoIPMessage msg = message::makeDiagnosticMessage(m_sourceAddress, m_logicalAddress, payload);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(msg));

    return write(_sockFd, msg.data(), msg.size());
}

ssize_t DoIPClient::sendAliveCheckResponse() {
    DoIPMessage msg = message::makeAliveCheckResponse(m_sourceAddress);
    LOG_DOIP_INFO("TX: {}", fmt::streamed(msg));
    return write(_sockFd, msg.data(), msg.size());
}

/*
 * Receive a message from server
 */
void DoIPClient::receiveMessage() {

    ssize_t bytesRead = recv(_sockFd, _receivedData, _maxDataSize, 0);

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

    auto optMmsg = DoIPMessage::tryParse(_receivedData, static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        LOG_DOIP_ERROR("Failed to parse DoIP message from received data");
        return;
    }
    DoIPMessage msg = optMmsg.value();
    LOG_TCP_INFO("RX: {}", fmt::streamed(msg));
}

void DoIPClient::receiveUdpMessage() {

    unsigned int length = sizeof(_clientAddr);

    // Set socket to timeout after 3 seconds
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(_sockFd_udp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int bytesRead;
    bytesRead = recvfrom(_sockFd_udp, _receivedData, _maxDataSize, 0, reinterpret_cast<struct sockaddr *>(&_clientAddr), &length);

    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            LOG_UDP_WARN("Timeout waiting for UDP response");
        } else {
            LOG_UDP_ERROR("Error receiving UDP message: {}", strerror(errno));
        }
        return;
    }

    LOG_UDP_INFO("Received {} bytes from UDP", bytesRead);

    auto optMmsg = DoIPMessage::tryParse(_receivedData, static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        LOG_UDP_ERROR("Failed to parse DoIP message from UDP data");
        return;
    }

    DoIPMessage msg = optMmsg.value();

    LOG_UDP_INFO("RX: {}", fmt::streamed(msg));
}

bool DoIPClient::receiveVehicleAnnouncement() {
    unsigned int length = sizeof(_announcementAddr);
    int bytesRead;

    LOG_UDP_DEBUG("Listening for Vehicle Announcements on port {}", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

    // Set socket to non-blocking mode for timeout
    struct timeval timeout;
    timeout.tv_sec = 2; // 2 second timeout
    timeout.tv_usec = 0;
    setsockopt(_sockFd_announcement, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    bytesRead = recvfrom(_sockFd_announcement, _receivedData, _maxDataSize, 0,
                         reinterpret_cast<struct sockaddr *>(&_announcementAddr), &length);
    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            LOG_UDP_WARN("Timeout waiting for Vehicle Announcement");
        } else {
            LOG_UDP_ERROR("Error receiving Vehicle Announcement: {}", strerror(errno));
        }
        return false;
    }

    auto optMsg = DoIPMessage::tryParse(_receivedData, static_cast<size_t>(bytesRead));
    if (!optMsg.has_value()) {
        LOG_UDP_ERROR("Failed to parse Vehicle Announcement message");
        return false;
    }

    DoIPMessage msg = optMsg.value();
    LOG_UDP_INFO("Vehicle Announcement received: {}", fmt::streamed(msg));

    // Parse and display the announcement information
    if (msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse) {
        parseVIResponseInformation(msg.data());
        displayVIResponseInformation();
        return true;
    }
    return false;
}

ssize_t DoIPClient::sendVehicleIdentificationRequest(const char *inet_address) {

    int setAddressError = inet_aton(inet_address, &(_serverAddr.sin_addr));

    if (setAddressError != 0) {
        LOG_UDP_INFO("Address set successfully");
    } else {
        LOG_UDP_ERROR("Could not set address. Try again");
    }

    int socketError = setsockopt(_sockFd_udp, SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));

    if (socketError == 0) {
        LOG_UDP_INFO("Broadcast Option set successfully");
    }

    DoIPMessage vehicleIdReq = message::makeVehicleIdentificationRequest();

    ssize_t bytesSent = sendto(_sockFd_udp, vehicleIdReq.data(), vehicleIdReq.size(), 0, reinterpret_cast<struct sockaddr *>(&_serverAddr), sizeof(_serverAddr));
    LOG_UDP_INFO("Sent Vehicle Identification Request to {}:{}", inet_address, ntohs(_serverAddr.sin_port));

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
    return _sockFd;
}

/*
 * Getter for _connected
 */
int DoIPClient::getConnected() {
    return _connected;
}

void DoIPClient::parseVIResponseInformation(const uint8_t *data) {

    // VIN
    int j = 0;
    for (int i = 8; i <= 24; i++) {
        VINResult[j] = data[i];
        j++;
    }

    // Logical Adress
    m_logicalAddress.update(data, 25);

    // EID
    j = 0;
    for (int i = 27; i <= 32; i++) {
        EIDResult[j] = data[i];
        j++;
    }

    // GID
    j = 0;
    for (int i = 33; i <= 38; i++) {
        GIDResult[j] = data[i];
        j++;
    }

    // FurtherActionRequest
    FurtherActionReqResult = data[39];
}

void DoIPClient::displayVIResponseInformation() {
    std::ostringstream ss;
    // output VIN
    ss << "VIN: ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    for (int i = 0; i < 17; i++) {
        ss << VINResult[i];
    }
    if (Logger::colorsSupported()) {
        ss << ansi::reset;
    }
    LOG_DOIP_INFO(ss.str());

    // output LogicalAddress
    ss = std::ostringstream{};
    ss << "LogicalAddress: ";
    ss << m_logicalAddress;
    LOG_DOIP_INFO(ss.str());

    // output EID
    ss = std::ostringstream{};
    ss << "EID: ";
    if (Logger::colorsSupported()) {
        ss << ansi::bold_green;
    }
    for (int i = 0; i < 6; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << +EIDResult[i] << std::dec;
    }
    if (Logger::colorsSupported()) {
        ss << ansi::reset;
    }
    LOG_DOIP_INFO(ss.str());

    // output GID
    ss = std::ostringstream{};
    ss << "GID: ";
    for (int i = 0; i < 6; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << +GIDResult[i] << std::dec;
    }
    LOG_DOIP_INFO(ss.str());

    // output FurtherActionRequest
    ss = std::ostringstream{};
    ss << "FurtherActionRequest: ";
    ss << std::hex << std::setfill('0') << std::setw(2) << FurtherActionReqResult << std::dec;
    LOG_DOIP_INFO(ss.str());
}

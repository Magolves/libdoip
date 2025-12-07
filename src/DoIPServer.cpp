#include "DoIPServer.h"
#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPServerModel.h"
#include "Logger.h"
#include "MacAddress.h"
#include <algorithm> // for std::remove_if
#include <cerrno>    // for errno
#include <cstring>   // for strerror

using namespace doip;

#if defined(__linux__)
const char *DEFAULT_IFACE = "eth0";
#elif defined(__APPLE__)
const char *DEFAULT_IFACE = "en0";
#else
#pragma error "Unsupported platform"
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

DoIPServer::~DoIPServer() {
    if (m_running.load()) {
        stop();
    }
}

DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config) {
    m_receiveBuf.reserve(MAX_ISOTP_MTU);

    // Apply VIN/EID/GID from config (types enforce size/padding)
    m_VIN = m_config.vin;
    m_EID = m_config.eid;
    m_GID = m_config.gid;

    // Set logical gateway address from config
    m_gatewayAddress.update(m_config.logicalAddress);

    // Apply EID/GID if provided (interpreted as numeric where possible)
    // (old parsing logic removed - ServerConfig uses typed identifiers)
    if (!m_config.loopback) {
        setAnnouncementMode(false);
    } else {
        setAnnouncementMode(true);
    }

    if (m_config.daemonize) {
        daemonize();
    }
}

void DoIPServer::daemonize() {
    LOG_DOIP_INFO("Daemonizing DoIP Server...");
    pid_t pid = fork();
    if (pid < 0) {
        LOG_DOIP_ERROR("First fork failed: {}", strerror(errno));
        return;
    }

    if (pid > 0) {
        // Parent exits; child continues
        _exit(0);
    }

    // Child: create new session and become session leader
    if (setsid() < 0) {
        LOG_DOIP_ERROR("setsid failed: {}", strerror(errno));
        return;
    }

    // Second fork to ensure the daemon can't reacquire a tty
    pid = fork();
    if (pid < 0) {
        LOG_DOIP_ERROR("Second fork failed: {}", strerror(errno));
        return;
    }
    if (pid > 0) {
        _exit(0);
    }

    // Set file mode creation mask to a safe default
    umask(0);

    // Change working directory to root to avoid blocking mounts
    if (chdir("/") != 0) {
        LOG_DOIP_WARN("chdir to / failed: {}", strerror(errno));
    }

    // Close and redirect standard file descriptors to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    } else {
        LOG_DOIP_WARN("Failed to open /dev/null: {}", strerror(errno));
    }

    LOG_DOIP_INFO("DoIP Server daemonized and running");
}

/*
 * Stop the server and cleanup
 */
void DoIPServer::stop() {
    LOG_DOIP_INFO("Stopping DoIP Server...");
    m_running.store(false);

    // Close sockets to unblock any pending accept/recv calls
    closeUdpSocket();
    closeTcpSocket();

    // Wait for all threads to finish
    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_workerThreads.clear();

    LOG_DOIP_INFO("DoIP Server stopped");
}

/*
 * Background thread: UDP message listener
 */
void DoIPServer::udpListenerThread() {
    LOG_DOIP_INFO("UDP listener thread started");

    while (m_running.load()) {
        ssize_t result = receiveUdpMessage();

        // If timeout (result == 0), continue without delay
        // The socket already has a timeout configured
        if (result < 0 && m_running.load()) {
            // Only log errors if we're still supposed to be running
            LOG_UDP_DEBUG("UDP receive error, continuing...");
        }
    }

    LOG_DOIP_INFO("UDP listener thread stopped");
}

/*
 * Background thread: Handle individual TCP connection
 */
void DoIPServer::connectionHandlerThread(std::unique_ptr<DoIPConnection> connection) {
    LOG_TCP_INFO("Connection handler thread started");

    while (m_running.load() && connection->isSocketActive()) {
        int result = connection->receiveTcpMessage();

        if (result < 0) {
            LOG_TCP_INFO("Connection closed or error occurred");
            break;
        }
    }

    // Connection is automatically closed when unique_ptr goes out of scope
    LOG_TCP_INFO("Connection handler thread stopped");
}

/*
 * Set up a tcp socket, so the socket is ready to accept a connection
 */
bool DoIPServer::setupTcpSocket() {
    LOG_DOIP_DEBUG("Setting up TCP socket on port {}", DOIP_SERVER_PORT);

    m_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcp_sock < 0) {
        LOG_TCP_ERROR("Failed to create TCP socket: {}", strerror(errno));
        return false;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(m_tcp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOG_TCP_WARN("Failed to set SO_REUSEADDR: {}", strerror(errno));
    }

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_PORT);

    // binds the socket to the address and port number
    if (bind(m_tcp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        LOG_TCP_ERROR("Failed to bind TCP socket: {}", strerror(errno));
        return false;
    }

    LOG_TCP_INFO("TCP socket successfully bound to port {}", DOIP_SERVER_PORT);
    return true;
}

bool DoIPServer::setupUdpSocket() {
    LOG_UDP_DEBUG("Setting up UDP socket on port {}", DOIP_UDP_DISCOVERY_PORT);

    m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);

    if (m_udp_sock < 0) {
        LOG_UDP_ERROR("Failed to create UDP socket: {}", strerror(errno));
        return false;
    }

    // binds the socket to any IP DoIPAddress and the Port Number 13400
    if (bind(m_udp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        LOG_UDP_ERROR("Failed to bind UDP socket: {}", strerror(errno));
        return false;
    }

    // setting the IP DoIPAddress for Multicast/Broadcast
    if (!m_useLoopbackAnnouncements) { //
        setMulticastGroup("224.0.0.2");
        LOG_UDP_INFO("UDP socket successfully bound to port {} with multicast group", DOIP_UDP_DISCOVERY_PORT);
    } else {
        LOG_UDP_INFO("UDP socket successfully bound to port {} with broadcast", DOIP_UDP_DISCOVERY_PORT);
    }
    LOG_UDP_DEBUG(
        "Socket {} bound to {}:{}",
        m_udp_sock,
        inet_ntoa(m_serverAddress.sin_addr),
        ntohs(m_serverAddress.sin_port));
    return true;
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
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(m_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in clientAddr;
    socklen_t length = sizeof(clientAddr);

    const ssize_t readBytes = recvfrom(
        m_udp_sock,
        m_receiveBuf.data(),
        m_receiveBuf.size(),
        0,
        reinterpret_cast<struct sockaddr *>(&clientAddr),
        &length);

    const int recvErrno = errno; // Save errno immediately
    // LOG_UDP_DEBUG("recvfrom returned: {} (errno: {})", readBytes, recvErrno);

    if (readBytes <= 0) {
        if (recvErrno == EAGAIN) { // EAGAIN sufficient: error: logical ‘or’ of equal expressions [-Werror=logical-op]  271 |         if (recvErrno == EAGAIN || recvErrno == EWOULDBLOCK) {
            return 0;              // Timeout (no sleep here)
        } else {
            LOG_UDP_ERROR("Error receiving UDP message: {}", strerror(recvErrno));
            return -1;
        }
    }

    LOG_UDP_INFO("RX {} bytes from {}:{}",
                 readBytes,
                 inet_ntoa(clientAddr.sin_addr),
                 ntohs(clientAddr.sin_port));

    m_clientAddress = clientAddr;
    return reactToReceivedUdpMessage(static_cast<size_t>(readBytes));
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
    // auto payloadLength = optHeader->second;
    LOG_UDP_INFO("RX: {}", fmt::streamed(plType));
    switch (plType) {
    case DoIPPayloadType::VehicleIdentificationRequest: {
        DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID);
        LOG_DOIP_INFO("TX {}", fmt::streamed(msg));
        ssize_t sendedBytes = sendUdpMessage(msg.data(), DOIP_HEADER_SIZE + msg.size());

        return static_cast<int>(sendedBytes);
    }

    default: {
        LOG_DOIP_ERROR("Invalid payload type 0x{:04X} received (receiveUdpMessage())", static_cast<uint16_t>(plType));
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
        LOG_DOIP_ERROR("Failed to get MAC address, using default EID");
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

void DoIPServer::setVIN(const DoIPVIN &vin) {
    m_VIN = vin;
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

void DoIPServer::setAnnouncementMode(bool useLoopback) {
    m_useLoopbackAnnouncements = useLoopback;
    if (useLoopback) {
        LOG_DOIP_INFO("Vehicle announcements will use loopback (127.0.0.1)");
    } else {
        LOG_DOIP_INFO("Vehicle announcements will use broadcast (255.255.255.255)");
    }
}

void DoIPServer::setMulticastGroup(const char *address) {
    int loop = 1;

    // set Option using the same Port for multiple Sockets
    int setPort = setsockopt(m_udp_sock, SOL_SOCKET, SO_REUSEADDR, &loop, sizeof(loop));

    if (setPort < 0) {
        LOG_UDP_ERROR("Setting Port Error");
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    // set Option to join Multicast Group
    int setGroup = setsockopt(m_udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mreq), sizeof(mreq));

    if (setGroup < 0) {
        LOG_UDP_ERROR("Setting address failed: {}", strerror(errno));
    }
}

ssize_t DoIPServer::sendVehicleAnnouncement() {

    // Choose address based on mode
    const char *address = m_useLoopbackAnnouncements ? "127.0.0.1" : "255.255.255.255";

    // setting the destination port for the Announcement to 13401
    m_clientAddress.sin_port = htons(13401);

    int setAddressError = inet_aton(address, &(m_clientAddress.sin_addr));

    if (setAddressError != 0) {
        LOG_DOIP_INFO("{} address set successfully: {}",
                      m_useLoopbackAnnouncements ? "Loopback" : "Broadcast", address);
    }

    if (!m_useLoopbackAnnouncements) {
        // Only set broadcast option for broadcast mode
        int socketError = setsockopt(m_udp_sock, SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));
        if (socketError == 0) {
            LOG_DOIP_INFO("Broadcast Option set successfully");
        }
    }

    ssize_t sentBytes = -1;

    // uint8_t *message = createVehicleIdentificationResponse(VIN, m_gatewayAddress, m_EID, GID, FurtherActionReq);
    DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID, m_FurtherActionReq);

    for (int i = 0; i < m_announceNum; i++) {
        sentBytes = sendto(m_udp_sock, msg.data(), msg.size(), 0, reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));

        if (sentBytes > 0) {
            LOG_UDP_INFO("Sent Vehicle Announcement {}/{}: {} bytes to {}:{}", i + 1, m_announceNum, sentBytes,
                         inet_ntoa(m_clientAddress.sin_addr), ntohs(m_clientAddress.sin_port));
        } else {
            LOG_UDP_ERROR("Failed sending Vehicle Announcement: {}", strerror(errno));
            LOG_UDP_ERROR("Message: {}", fmt::streamed(msg));
        }
        usleep(m_announceInterval * 1000);
    }
    return sentBytes;
}

ssize_t DoIPServer::sendNegativeUdpAck(DoIPNegativeAck ackCode) {
    DoIPMessage message = message::makeNegativeAckMessage(ackCode);
    return sendUdpMessage(message.data(), message.size());
}

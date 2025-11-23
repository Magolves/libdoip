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

DoIPServer::~DoIPServer() {
    if (m_running.load()) {
        stop();
    }
}

/*
 * High-level API: Start the server with automatic connection handling
 */
bool DoIPServer::start(ConnectionAcceptedHandler onConnectionAccepted, bool sendAnnouncements) {
    if (m_running.load()) {
        DOIP_LOG_WARN("Server is already running");
        return false;
    }

    if (!onConnectionAccepted) {
        DOIP_LOG_ERROR("Connection handler callback is required");
        return false;
    }

    m_connectionHandler = onConnectionAccepted;

    // Setup sockets
    setupUdpSocket();
    if (m_udp_sock < 0) {
        DOIP_LOG_ERROR("Failed to setup UDP socket");
        return false;
    }

    setupTcpSocket();
    if (m_tcp_sock < 0) {
        DOIP_LOG_ERROR("Failed to setup TCP socket");
        closeUdpSocket();
        return false;
    }

    // Start background threads
    m_running.store(true);

    try {
        m_workerThreads.emplace_back(&DoIPServer::udpListenerThread, this);
        m_workerThreads.emplace_back(&DoIPServer::tcpListenerThread, this);

        DOIP_LOG_INFO("DoIP Server started successfully");

        // Send vehicle announcements if requested
        if (sendAnnouncements) {
            sendVehicleAnnouncement();
        }

        return true;
    } catch (const std::exception &e) {
        DOIP_LOG_ERROR("Failed to start worker threads: {}", e.what());
        m_running.store(false);
        closeUdpSocket();
        closeTcpSocket();
        return false;
    }
}

/*
 * High-level API: Stop the server and cleanup
 */
void DoIPServer::stop() {
    DOIP_LOG_INFO("Stopping DoIP Server...");
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

    DOIP_LOG_INFO("DoIP Server stopped");
}

/*
 * Background thread: UDP message listener
 */
void DoIPServer::udpListenerThread() {
    DOIP_LOG_INFO("UDP listener thread started");

    while (m_running.load()) {
        ssize_t result = receiveUdpMessage();

        // If timeout (result == 0), continue without delay
        // The socket already has a timeout configured
        if (result < 0 && m_running.load()) {
            // Only log errors if we're still supposed to be running
            UDP_LOG_DEBUG("UDP receive error, continuing...");
        }
    }

    DOIP_LOG_INFO("UDP listener thread stopped");
}

/*
 * Background thread: TCP connection acceptor
 */
void DoIPServer::tcpListenerThread() {
    DOIP_LOG_INFO("TCP listener thread started");

    while (m_running.load()) {
        auto connection = waitForTcpConnection();

        if (!connection) {
            if (m_running.load()) {
                TCP_LOG_DEBUG("Failed to accept connection, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Invoke the user's callback to get the server model
        auto optModel = m_connectionHandler(connection.get());

        if (!optModel.has_value()) {
            TCP_LOG_INFO("Connection rejected by application handler");
            continue;
        }

        // Apply the model to the connection
        connection->setServerModel(optModel.value());

        // Spawn a dedicated thread for this connection
        // Note: We detach because the connection thread manages its own lifecycle
        std::thread(&DoIPServer::connectionHandlerThread, this, std::move(connection)).detach();
    }

    DOIP_LOG_INFO("TCP listener thread stopped");
}

/*
 * Background thread: Handle individual TCP connection
 */
void DoIPServer::connectionHandlerThread(std::unique_ptr<DoIPConnection> connection) {
    TCP_LOG_INFO("Connection handler thread started");

    while (m_running.load() && connection->isSocketActive()) {
        int result = connection->receiveTcpMessage();

        if (result < 0) {
            TCP_LOG_INFO("Connection closed or error occurred");
            break;
        }
    }

    // Connection is automatically closed when unique_ptr goes out of scope
    TCP_LOG_INFO("Connection handler thread stopped");
}

/*
 * Set up a tcp socket, so the socket is ready to accept a connection
 */
void DoIPServer::setupTcpSocket() {
    DOIP_LOG_DEBUG("Setting up TCP socket on port {}", DOIP_SERVER_PORT);

    m_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcp_sock < 0) {
        TCP_LOG_ERROR("Failed to create TCP socket: {}", strerror(errno));
        return;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(m_tcp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        TCP_LOG_WARN("Failed to set SO_REUSEADDR: {}", strerror(errno));
    }

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_PORT);

    // binds the socket to the address and port number
    if (bind(m_tcp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        TCP_LOG_ERROR("Failed to bind TCP socket: {}", strerror(errno));
        return;
    }

    TCP_LOG_INFO("TCP socket successfully bound to port {}", DOIP_SERVER_PORT);
}

/*
 *  Wait till a client attempts a connection and accepts it
 */
template <typename Model>
std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection() {
    static_assert(std::is_default_constructible<Model>::value,
                  "Model must be default-constructible");

    TCP_LOG_INFO("Waiting for TCP connection...");

    // waits till client approach to make connection
    if (listen(m_tcp_sock, 5) < 0) {
        TCP_LOG_CRITICAL("Failed to listen on TCP socket: {}", strerror(errno));
        return nullptr;
    }

    int tcpSocket = accept(m_tcp_sock, nullptr, nullptr);
    if (tcpSocket < 0) {
        TCP_LOG_CRITICAL("Failed to accept TCP connection: {}", strerror(errno));
        return nullptr;
    }

    TCP_LOG_INFO("TCP connection accepted, socket: {}", tcpSocket);

    // Create a default server model with the gateway address
    Model model;
    model.serverAddress = m_gatewayAddress;
    model.onCloseConnection = []() noexcept {};
    model.onDiagnosticMessage = [](const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
        (void)msg;
        return std::nullopt;
    };
    model.onDiagnosticNotification = [](DoIPDiagnosticAck ack) noexcept {
        (void)ack;
    };

    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, model));
}

void DoIPServer::setupUdpSocket() {
    UDP_LOG_DEBUG("Setting up UDP socket on port {}", DOIP_SERVER_PORT);

    m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_PORT);

    if (m_udp_sock < 0) {
        UDP_LOG_ERROR("Failed to create UDP socket: {}", strerror(errno));
        return;
    }

    // binds the socket to any IP DoIPAddress and the Port Number 13400
    if (bind(m_udp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        UDP_LOG_ERROR("Failed to bind UDP socket: {}", strerror(errno));
        return;
    }

    // setting the IP DoIPAddress for Multicast
    setMulticastGroup("224.0.0.2");
    UDP_LOG_INFO("UDP socket successfully bound to port {} with multicast group", DOIP_SERVER_PORT);
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
    UDP_LOG_DEBUG("Wait for UDP message...");

    // Set socket timeout to prevent blocking indefinitely
    struct timeval timeout;
    timeout.tv_sec = 1; // 1 second timeout
    timeout.tv_usec = 0;
    setsockopt(m_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in clientAddr;
    unsigned int length = sizeof(clientAddr);
    const ssize_t readBytes = recvfrom(m_udp_sock, m_receiveBuf.data(), m_receiveBuf.size(), 0, reinterpret_cast<struct sockaddr *>(&clientAddr), &length);

    if (readBytes < 0) {
        if (errno == EAGAIN) {
            // Timeout - this is normal, just continue
            return 0;
        } else {
            UDP_LOG_ERROR("Error receiving UDP message: {}", strerror(errno));
            return -1;
        }
    }

    // Don't log if no data received (can happen with some socket configurations)
    if (readBytes > 0) {
        UDP_LOG_INFO("RX {} bytes from {}:{}", readBytes,
                     inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    } else {
        // For debugging: log zero-byte messages at debug level
        UDP_LOG_DEBUG("RX 0 bytes from {}:{}",
                      inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        return 0; // Return early for zero-byte messages
    }

    // Store client address for response
    m_clientAddress = clientAddr;

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
    // auto payloadLength = optHeader->second;
    UDP_LOG_INFO("RX: {}", fmt::streamed(plType));
    switch (plType) {
    case DoIPPayloadType::VehicleIdentificationRequest: {
        DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID);
        DOIP_LOG_PROTOCOL("TX {}", fmt::streamed(msg));
        ssize_t sendedBytes = sendUdpMessage(msg.data(), DOIP_HEADER_SIZE + msg.size());

        return static_cast<int>(sendedBytes);
    }

    default: {
        DOIP_LOG_ERROR("Invalid payload type 0x{:04X} received (receiveUdpMessage())", static_cast<uint16_t>(plType));
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
        DOIP_LOG_ERROR("Failed to get MAC address, using default EID");
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

void DoIPServer::setAnnouncementMode(bool useLoopback) {
    m_useLoopbackAnnouncements = useLoopback;
    if (useLoopback) {
        DOIP_LOG_INFO("Vehicle announcements will use loopback (127.0.0.1)");
    } else {
        DOIP_LOG_INFO("Vehicle announcements will use broadcast (255.255.255.255)");
    }
}

void DoIPServer::setMulticastGroup(const char *address) {
    int loop = 1;

    // set Option using the same Port for multiple Sockets
    int setPort = setsockopt(m_udp_sock, SOL_SOCKET, SO_REUSEADDR, &loop, sizeof(loop));

    if (setPort < 0) {
        UDP_LOG_ERROR("Setting Port Error");
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    // set Option to join Multicast Group
    int setGroup = setsockopt(m_udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mreq), sizeof(mreq));

    if (setGroup < 0) {
        UDP_LOG_ERROR("Setting address failed: {}", strerror(errno));
    }
}

ssize_t DoIPServer::sendVehicleAnnouncement() {

    // Choose address based on mode
    const char *address = m_useLoopbackAnnouncements ? "127.0.0.1" : "255.255.255.255";

    // setting the destination port for the Announcement to 13401
    m_clientAddress.sin_port = htons(13401);

    int setAddressError = inet_aton(address, &(m_clientAddress.sin_addr));

    if (setAddressError != 0) {
        DOIP_LOG_INFO("{} address set successfully: {}",
                      m_useLoopbackAnnouncements ? "Loopback" : "Broadcast", address);
    }

    if (!m_useLoopbackAnnouncements) {
        // Only set broadcast option for broadcast mode
        int socketError = setsockopt(m_udp_sock, SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));
        if (socketError == 0) {
            DOIP_LOG_INFO("Broadcast Option set successfully");
        }
    }

    ssize_t sentBytes = -1;

    // uint8_t *message = createVehicleIdentificationResponse(VIN, m_gatewayAddress, m_EID, GID, FurtherActionReq);
    DoIPMessage msg = message::makeVehicleIdentificationResponse(m_VIN, m_gatewayAddress, m_EID, m_GID, m_FurtherActionReq);

    for (int i = 0; i < m_announceNum; i++) {
        sentBytes = sendto(m_udp_sock, msg.data(), msg.size(), 0, reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));

        if (sentBytes > 0) {
            UDP_LOG_INFO("Sent Vehicle Announcement");
        } else {
            UDP_LOG_ERROR("Failed sending Vehicle Announcement: {}", strerror(errno));
            UDP_LOG_ERROR("Message: {}", fmt::streamed(msg));
        }
        usleep(m_announceInterval * 1000);
    }
    return sentBytes;
}

ssize_t DoIPServer::sendNegativeUdpAck(DoIPNegativeAck ackCode) {
    DoIPMessage message = message::makeNegativeAckMessage(ackCode);
    return sendUdpMessage(message.data(), message.size());
}

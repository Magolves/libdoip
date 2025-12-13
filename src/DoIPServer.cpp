#include <algorithm> // for std::remove_if
#include <cerrno>    // for errno
#include <cstring>   // for strerror
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPServer.h"
#include "DoIPServerModel.h"
#include "Logger.h"
#include "MacAddress.h"

using namespace doip;

DoIPServer::~DoIPServer() {
    if (m_running.load()) {
        stop();
    }
}

DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config) {
    m_receiveBuf.reserve(DOIP_MAXIMUM_MTU);

    setLoopbackMode(m_config.loopback);

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
bool DoIPServer::setupTcpSocket(std::function<UniqueServerModelPtr()> modelFactory) {
    LOG_DOIP_DEBUG("Setting up TCP socket on port {}", DOIP_SERVER_TCP_PORT);

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
    m_serverAddress.sin_port = htons(DOIP_SERVER_TCP_PORT);

    // binds the socket to the address and port number
    if (bind(m_tcp_sock, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        LOG_TCP_ERROR("Failed to bind TCP socket: {}", strerror(errno));
        return false;
    }

    // Put the socket into listening state so the OS reports LISTEN and accept() can proceed
    if (listen(m_tcp_sock, 5) < 0) {
        LOG_TCP_ERROR("Failed to listen on TCP socket: {}", strerror(errno));
        return false;
    }

    // Also start TCP acceptor thread so TCP 13400 enters LISTEN state and accepts connections
    m_workerThreads.emplace_back([this, modelFactory]() { tcpListenerThread(modelFactory); });

    LOG_TCP_INFO("TCP socket bound and listening on port {}", DOIP_SERVER_TCP_PORT);
    return true;
}

/*
 * Closes the socket for this server
 */
void DoIPServer::closeTcpSocket() {
    close(m_tcp_sock);
}

bool DoIPServer::setupUdpSocket() {
    LOG_UDP_DEBUG("Setting up UDP socket on port {}", DOIP_UDP_DISCOVERY_PORT);

    m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udp_sock < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // Set socket to non-blocking with timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(m_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Enable SO_REUSEADDR
    int reuse = 1;
    setsockopt(m_udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind socket to port 13400
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);

    if (bind(m_udp_sock, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(m_udp_sock);
        return 1;
    }
    // setting the IP DoIPAddress for Multicast/Broadcast
    if (!m_config.loopback) { //
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

    m_running.store(true);
    m_workerThreads.emplace_back([this]() { udpListenerThread(); });
    m_workerThreads.emplace_back([this]() { udpAnnouncementThread(); });

    return true;
}

void DoIPServer::closeUdpSocket() {
    m_running.store(false);
    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    close(m_udp_sock);
}

bool DoIPServer::setDefaultEid() {
    MacAddress mac = {0};
    if (!getFirstMacAddress(mac)) {
        LOG_DOIP_ERROR("Failed to get MAC address, using default EID");
        m_config.eid = DoIpEid::Zero;
        return false;
    }
    // Set EID based on MAC address (last 6 bytes)
    m_config.eid = DoIpEid(mac.data(), m_config.eid.ID_LENGTH);
    return true;
}

void DoIPServer::setVin(const std::string &VINString) {

    m_config.vin = DoIpVin(VINString);
}

void DoIPServer::setVin(const DoIpVin &vin) {
    if (!isValidVin(vin)) {
        LOG_DOIP_WARN("Invalid VIN provided {}", fmt::streamed(vin));
    }
    m_config.vin = vin;
}

void DoIPServer::setLogicalGatewayAddress(DoIPAddress logicalAddress) {
    m_config.logicalAddress = logicalAddress;
}

void DoIPServer::setEid(const uint64_t inputEID) {
    m_config.eid = DoIpEid(inputEID);
}

void DoIPServer::setGid(const uint64_t inputGID) {
    m_config.gid = DoIpGid(inputGID);
}

void DoIPServer::setFurtherActionRequired(DoIPFurtherAction furtherActionRequired) {
    m_FurtherActionReq = furtherActionRequired;
}

void DoIPServer::setAnnounceNum(int Num) {
    m_config.announceCount = Num;
}

void DoIPServer::setAnnounceInterval(unsigned int Interval) {
    m_config.announceInterval = Interval;
}

void DoIPServer::setLoopbackMode(bool useLoopback) {
    m_config.loopback = useLoopback;
    if (m_config.loopback) {
        LOG_DOIP_INFO("Vehicle announcements will use loopback (127.0.0.1)");
    } else {
        LOG_DOIP_INFO("Vehicle announcements will use broadcast (255.255.255.255)");
    }
}

void DoIPServer::setMulticastGroup(const char *address) const {
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
    int setGroup = setsockopt(m_udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char *>(&mreq), sizeof(mreq));

    if (setGroup < 0) {
        LOG_UDP_ERROR("Setting address failed: {}", strerror(errno));
    }
}

ssize_t DoIPServer::sendNegativeUdpAck(DoIPNegativeAck ackCode) {
    DoIPMessage msg = message::makeNegativeAckMessage(ackCode);

    return sendUdpResponse(msg);
}

// new version starts here
void DoIPServer::udpListenerThread() {
    socklen_t client_len = sizeof(m_clientAddress);

    LOG_UDP_INFO("UDP listener thread started");

    while (m_running) {
        ssize_t received = recvfrom(m_udp_sock, m_receiveBuf.data(), sizeof(m_receiveBuf), 0,
                                    reinterpret_cast<struct sockaddr *>(&m_clientAddress), &client_len);

        if (received < 0) {
            if (errno == EAGAIN /* || errno == EWOULDBLOCK*/) {
                // Timeout, continue
                continue;
            }
            if (m_running) {
                perror("recvfrom error");
            }
            break;
        }

        if (received > 0) {
            std::scoped_lock lock(m_mutex);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &m_clientAddress.sin_addr, client_ip, sizeof(client_ip));
            m_clientIp = std::string(client_ip);
            m_clientPort = ntohs(m_clientAddress.sin_port);

            LOG_UDP_INFO("Received {} bytes from {}:{}", received, m_clientIp, m_clientPort);

            auto optHeader = DoIPMessage::tryParseHeader(m_receiveBuf.data(), DOIP_HEADER_SIZE);
            if (!optHeader.has_value()) {
                auto sentBytes = sendNegativeUdpAck(DoIPNegativeAck::IncorrectPatternFormat);
                if (sentBytes < 0) {
                    if (errno == EAGAIN /*|| errno == EWOULDBLOCK*/) {
                        usleep(100);
                        continue;
                    }
                    break;
                }
            }
            auto plType = optHeader->first;
            // auto payloadLength = optHeader->second;
            LOG_UDP_INFO("RX: {}", fmt::streamed(plType));

            ssize_t sentBytes = 0;
            switch (plType) {
            case DoIPPayloadType::VehicleIdentificationRequest: {
                DoIPMessage msg = message::makeVehicleIdentificationResponse(m_config.vin, m_config.logicalAddress, m_config.eid, m_config.gid);
                sentBytes = sendUdpResponse(msg);
            } break;

            default:
                LOG_DOIP_ERROR("Invalid payload type 0x{:04X} received (receiveUdpMessage())", static_cast<uint16_t>(plType));
                sentBytes = sendNegativeUdpAck(DoIPNegativeAck::UnknownPayloadType);

            } // switch

            if (sentBytes < 0) {
                if (errno == EAGAIN /*|| errno == EWOULDBLOCK*/) {
                    usleep(100);
                    continue;
                }
                break;
            }
        }
    }

    LOG_UDP_INFO("UDP listener thread stopped");
}

void DoIPServer::udpAnnouncementThread() {
    LOG_DOIP_INFO("Announcement thread started");

    // Send announcements with configured interval and count
    for (int i = 0; i < m_config.announceCount && m_running; i++) {
        sendVehicleAnnouncement();
        usleep(m_config.announceInterval * 1000);
    }

    LOG_DOIP_INFO("Announcement thread stopped");
}

ssize_t DoIPServer::sendVehicleAnnouncement() {
    DoIPMessage msg = message::makeVehicleIdentificationResponse(m_config.vin, m_config.logicalAddress, m_config.eid, m_config.gid);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

    const char *dest_ip;
    if (m_config.loopback) {
        dest_ip = "127.0.0.1";
        inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);
    } else {
        dest_ip = "255.255.255.255";
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        // Enable broadcast
        int broadcast = 1;
        setsockopt(m_udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    }

    ssize_t sentBytes = sendto(m_udp_sock, msg.data(), msg.size(), 0,
                               reinterpret_cast<struct sockaddr *>(&dest_addr), sizeof(dest_addr));

    LOG_DOIP_INFO("TX {}", fmt::streamed(msg));
    if (sentBytes > 0) {
        LOG_UDP_INFO("Sent Vehicle Announcement: {} bytes to {}:{}",
                     sentBytes, dest_ip, DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
    } else {
        LOG_UDP_ERROR("Failed to send announcement: {}", strerror(errno));
    }
    return sentBytes;
}

ssize_t DoIPServer::sendUdpResponse(DoIPMessage msg) {
    auto sentBytes = sendto(m_udp_sock, msg.data(), msg.size(), 0,
                            reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));

    if (sentBytes > 0) {
        LOG_DOIP_INFO("TX {}", fmt::streamed(msg));
        LOG_UDP_INFO("Sent UDS response: {} bytes to {}:{}",
                     sentBytes, m_clientIp, ntohs(m_clientAddress.sin_port));
    } else {
        LOG_DOIP_ERROR("Failed to send message: {}", strerror(errno));
    }
    return sentBytes;
}

std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection(UniqueServerModelPtr model) {
    // waits till client approach to make connection
    if (listen(m_tcp_sock, 5) < 0) {
        return nullptr;
    }

    int tcpSocket = accept(m_tcp_sock, nullptr, nullptr);
    if (tcpSocket < 0) {
        return nullptr;
    }

    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, std::move(model)));
}

void DoIPServer::tcpListenerThread(std::function<UniqueServerModelPtr()> modelFactory) {
    LOG_DOIP_INFO("TCP listener thread started");

    while (m_running.load()) {
        auto model = modelFactory ? modelFactory() : std::make_unique<DefaultDoIPServerModel>();
        auto connection = waitForTcpConnection(std::move(model));

        if (!connection) {
            if (m_running.load()) {
                LOG_TCP_DEBUG("Failed to accept connection, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Spawn a dedicated thread for this connection
        // Note: We detach because the connection thread manages its own lifecycle
        std::thread(&DoIPServer::connectionHandlerThread, this, std::move(connection)).detach();
    }

    LOG_DOIP_INFO("TCP listener thread stopped");
}

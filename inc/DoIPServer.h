#ifndef DOIPSERVER_H
#define DOIPSERVER_H

#include <arpa/inet.h>
#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <net/if.h>
#include <netinet/in.h>
#include <optional>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "ByteArray.h"
#include "DoIPConfig.h"
#include "DoIPConnection.h"
#include "DoIPFurtherAction.h"
#include "DoIPIdentifiers.h"
#include "DoIPNegativeAck.h"
#include "DoIPServerModel.h"
#include "MacAddress.h"

namespace doip {

/**
 * @brief Server configuration structure used to initialize a DoIP server.
 */
struct ServerConfig {
    // EID and GID as fixed identifiers (6 bytes). Default: zeros.
    DoIPEID eid = DoIPEID::Zero;
    DoIPGID gid = DoIPGID::Zero;

    // VIN as fixed identifier (17 bytes). Default: zeros.
    DoIPVIN vin = DoIPVIN::Zero;

    // Logical/server address (default 0x0E00)
    DoIPAddress logicalAddress = DoIPAddress(0x0028);

    // Use loopback announcements instead of broadcast
    bool loopback = false;

    // Run the server as a daemon by default
    bool daemonize = false;

    int announceCount = 3;               // Default Value = 3
    unsigned int announceInterval = 500; // Default Value = 500ms
};

const ServerConfig DefaultServerConfig{};

constexpr int DOIP_SERVER_TCP_PORT = 13400;

/**
 * @brief Callback invoked when a new TCP connection is established
 * @return DoIPServerModel to use for this connection, or std::nullopt to reject
 */
using ConnectionAcceptedHandler = std::function<std::optional<DoIPServerModel>(DoIPConnection *)>;

/**
 * @brief DoIP Server class to handle incoming DoIP connections and UDP messages.
 *
 * This class manages the low-level TCP/UDP socket handling.
 */
class DoIPServer {

  public:
    /**
     * @brief Construct a DoIP server with the given configuration.
     * @param config Server configuration (EID/GID/VIN, announce params, etc.).
     */
    explicit DoIPServer(const ServerConfig &config = DefaultServerConfig);

    /**
     * @brief Destructor. Ensures sockets/threads are closed/stopped.
     */
    ~DoIPServer();

    DoIPServer(const DoIPServer &) = delete;
    DoIPServer &operator=(const DoIPServer &) = delete;
    DoIPServer(DoIPServer &&) = delete;
    DoIPServer &operator=(DoIPServer &&) = delete;

    [[nodiscard]]
    /**
     * @brief Initialize and bind the TCP socket for DoIP.
     * @return true on success, false otherwise.
     */
    bool setupTcpSocket();

    template <typename Model = DefaultDoIPServerModel>
    /**
     * @brief Block until a TCP client connects and create a DoIP connection.
     * @tparam Model Server model type used by the connection (default `DefaultDoIPServerModel`).
     * @return Unique pointer to established `DoIPConnection`, or nullptr on failure.
     */
    std::unique_ptr<DoIPConnection> waitForTcpConnection();

    [[nodiscard]]
    /**
     * @brief Initialize and bind the UDP socket for announcements and UDP messages.
     * @return true on success, false otherwise.
     */
    bool setupUdpSocket();

    /**
     * @brief Check if the server is currently running
     */
    [[nodiscard]]
    bool isRunning() const { return m_running.load(); }

    /**
     * @brief Set the number of vehicle announcements to send.
     * @param Num Count of announcements.
     */
    void setAnnounceNum(int Num);
    /**
     * @brief Set the interval between announcements in milliseconds.
     * @param Interval Interval in ms.
     */
    void setAnnounceInterval(unsigned int Interval);
    /**
     * @brief Enable/disable loopback mode for announcements (no broadcast).
     * @param useLoopback True to use loopback, false for broadcast.
     */
    void setLoopbackMode(bool useLoopback);

    /**
     * @brief Close the TCP socket if open.
     */
    void closeTcpSocket();
    /**
     * @brief Close the UDP socket if open.
     */
    void closeUdpSocket();

    /**
     * @brief Set the logical DoIP gateway address.
     * @param logicalAddress Logical address value.
     */
    void setLogicalGatewayAddress(unsigned short logicalAddress);

    /**
     * @brief Sets the EID to a default value based on the MAC address.
     *
     * @return true if the EID was successfully set to the default value.
     * @return false if the default EID could not be set.
     */
    bool setDefaultEid();

    /**
     * @brief Set VIN from a 17-character string.
     * @param VINString VIN string (17 bytes expected).
     */
    void setVin(const std::string &VINString);
    /**
     * @brief Set VIN from a `DoIPVIN` instance.
     * @param vin VIN value.
     */
    void setVin(const DoIPVIN &vin);
    /**
     * @brief Get current VIN.
     * @return Reference to configured VIN.
     */
    const DoIPVIN &getVin() const { return m_config.vin; }

    /**
     * @brief Set EID value.
     * @param nputEID EID as 64-bit value (lower 48 bits used).
     */
    void setEid(uint64_t nputEID);
    /**
     * @brief Get current EID.
     * @return Reference to configured EID.
     */
    const DoIPEID &getEid() const { return m_config.eid; }

    /**
     * @brief Set GID value.
     * @param inputGID GID as 64-bit value (lower 48 bits used).
     */
    void setGid(uint64_t inputGID);
    /**
     * @brief Get current GID.
     * @return Reference to configured GID.
     */
    const DoIPGID &getGid() const { return m_config.gid; }

    /**
     * @brief Get current further action requirement status.
     * @return Current `DoIPFurtherAction` value.
     */
    DoIPFurtherAction getFurtherActionRequired() const { return m_FurtherActionReq; }
    /**
     * @brief Set further action requirement status.
     * @param furtherActionRequired Value to set.
     */
    void setFurtherActionRequired(DoIPFurtherAction furtherActionRequired);

    /**
     * @brief Get last accepted client IP (string form).
     * @return IP address string.
     */
    std::string getClientIp() const { return m_clientIp; }
    /**
     * @brief Get last accepted client TCP port.
     * @return Client port number.
     */
    int getClientPort() const { return m_clientPort; }

  private:
    int m_tcp_sock{-1};
    int m_udp_sock{-1};
    struct sockaddr_in m_serverAddress{};
    struct sockaddr_in m_clientAddress{};
    ByteArray m_receiveBuf{};
    std::string m_clientIp{};
    int m_clientPort{};
    DoIPFurtherAction m_FurtherActionReq = DoIPFurtherAction::NoFurtherAction;

    // Automatic mode state
    std::atomic<bool> m_running{false};
    std::vector<std::thread> m_workerThreads;
    std::mutex m_mutex;

    // Server configuration
    ServerConfig m_config;

    void stop();
    void daemonize();

    void setMulticastGroup(const char *address);

    ssize_t sendNegativeUdpAck(DoIPNegativeAck ackCode);

    template <typename Model>
    void tcpListenerThread();

    void connectionHandlerThread(std::unique_ptr<DoIPConnection> connection);

    void udpListenerThread();
    void udpAnnouncementThread();
    ssize_t sendVehicleAnnouncement();

    ssize_t sendUdpMessage(const uint8_t *message, size_t messageLength);
};

// Template implementation must be in header for external linkage
template <typename Model>
std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection() {
    static_assert(std::is_default_constructible<Model>::value,
                  "Model must be default-constructible");

    // waits till client approach to make connection
    if (listen(m_tcp_sock, 5) < 0) {
        return nullptr;
    }

    int tcpSocket = accept(m_tcp_sock, nullptr, nullptr);
    if (tcpSocket < 0) {
        return nullptr;
    }

    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, std::make_unique<Model>()));
}

/*
 * Background thread: TCP connection acceptor
 */
template <typename Model>
void DoIPServer::tcpListenerThread() {
    LOG_DOIP_INFO("TCP listener thread started");

    while (m_running.load()) {
        auto connection = waitForTcpConnection<Model>();

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

} // namespace doip

#endif /* DOIPSERVER_H */
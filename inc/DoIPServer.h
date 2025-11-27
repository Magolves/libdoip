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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "ByteArray.h"
#include "DoIPConnection.h"
#include "DoIPFurtherAction.h"
#include "DoIPIdentifiers.h"
#include "DoIPNegativeAck.h"
#include "DoIPServerModel.h"
#include "MacAddress.h"

namespace doip {

const int DOIP_SERVER_PORT = 13400;

/**
 * @brief Callback invoked when a new TCP connection is established
 * @return DoIPServerModel to use for this connection, or std::nullopt to reject
 */
using ConnectionAcceptedHandler = std::function<std::optional<DoIPServerModel>(DoIPConnection *)>;

/**
 * @brief DoIP Server class to handle incoming DoIP connections and UDP messages.
 *
 * This class manages the low-level TCP/UDP socket handling and provides a clean
 * callback-based interface for application logic. The server can operate in two modes:
 *
 * 1. Manual mode: User calls waitForTcpConnection() and manages the receive loop
 * 2. Automatic mode: User calls start() with a callback, server manages everything
 */
class DoIPServer {

  public:
    DoIPServer() {
        m_receiveBuf.reserve(MAX_ISOTP_MTU);
    };

    ~DoIPServer();

    DoIPServer(const DoIPServer &) = delete;
    DoIPServer &operator=(const DoIPServer &) = delete;
    DoIPServer(DoIPServer &&) = delete;
    DoIPServer &operator=(DoIPServer &&) = delete;

    // === Low-level API (manual mode) ===
    void setupTcpSocket();

    template <typename Model = DefaultDoIPServerModel>
    std::unique_ptr<DoIPConnection> waitForTcpConnection();

    void setupUdpSocket();
    ssize_t receiveUdpMessage();

    // === High-level API (automatic mode) ===
    /**
     * @brief Start the DoIP server with automatic connection handling
     *
     * This method sets up TCP and UDP sockets, spawns background threads to handle
     * UDP messages and TCP connections, and returns immediately. The server continues
     * running until stop() is called.
     *
     * @param onConnectionAccepted Callback invoked when a new client connects.
     *                             Return a DoIPServerModel to accept the connection,
     *                             or std::nullopt to reject it.
     * @param sendAnnouncements If true, send vehicle announcements on startup
     * @return true if server started successfully, false otherwise
     */
    template <typename Model>
    bool start(ConnectionAcceptedHandler onConnectionAccepted, bool sendAnnouncements = true);

    /**
     * @brief Stop the DoIP server and wait for all threads to terminate
     *
     * This gracefully shuts down all active connections and background threads.
     * Blocks until all cleanup is complete.
     */
    void stop();

    /**
     * @brief Check if the server is currently running
     */
    bool isRunning() const { return m_running.load(); }

    void setAnnounceNum(int Num);
    void setAnnounceInterval(unsigned int Interval);
    void setAnnouncementMode(bool useLoopback);

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

    void setVIN(const std::string &VINString);
    const DoIPVIN &getVIN() const { return m_VIN; }

    void setEID(const uint64_t inputEID);
    const DoIPEID &getEID() const { return m_EID; }

    void setGID(const uint64_t inputGID);
    const DoIPGID &getGID() const { return m_GID; }

    void setFAR(DoIPFurtherAction inputFAR);

  private:
    int m_tcp_sock{-1};
    int m_udp_sock{-1};
    struct sockaddr_in m_serverAddress {};
    struct sockaddr_in m_clientAddress {};
    ByteArray m_receiveBuf{};

    DoIPVIN m_VIN;
    DoIPAddress m_gatewayAddress = DoIPAddress::ZeroAddress;
    DoIPEID m_EID = DoIPEID::Zero;
    DoIPGID m_GID = DoIPGID::Zero;
    DoIPFurtherAction m_FurtherActionReq = DoIPFurtherAction::NoFurtherAction;
    //TimerManager m_timerManager{};

    int m_announceNum = 3;                   // Default Value = 3
    unsigned int m_announceInterval = 500;   // Default Value = 500ms
    bool m_useLoopbackAnnouncements = false; // Default: use broadcast

    int m_broadcast = 1;

    // Automatic mode state
    std::atomic<bool> m_running{false};
    std::vector<std::thread> m_workerThreads;
    ConnectionAcceptedHandler m_connectionHandler;

    ssize_t reactToReceivedUdpMessage(size_t bytesRead);
    ssize_t sendUdpMessage(const uint8_t *message, size_t messageLength);

    void setMulticastGroup(const char *address);

    ssize_t sendNegativeUdpAck(DoIPNegativeAck ackCode);

    // Worker thread functions
    void udpListenerThread();

    template <typename Model>
    void tcpListenerThread();

    void connectionHandlerThread(std::unique_ptr<DoIPConnection> connection);
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

    // Create a default server model with the gateway address
    // Model model;
    // model.serverAddress = m_gatewayAddress;
    // if (model.onDiagnosticMessage == nullptr) {
    //     model.onDiagnosticMessage = [](IConnectionContext& ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
    //         (void)ctx;
    //         (void)msg;
    //         DOIP_LOG_CRITICAL("Diagnostic message received on default-constructed Model");
    //         // Default: always ACK
    //         return std::nullopt;
    //     };
    // }

    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, std::make_unique<Model>()));
}

/*
 * Background thread: TCP connection acceptor
 */
template <typename Model>
void DoIPServer::tcpListenerThread() {
    DOIP_LOG_INFO("TCP listener thread started");

    while (m_running.load()) {
        auto connection = waitForTcpConnection<Model>();

        if (!connection) {
            if (m_running.load()) {
                TCP_LOG_DEBUG("Failed to accept connection, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Spawn a dedicated thread for this connection
        // Note: We detach because the connection thread manages its own lifecycle
        std::thread(&DoIPServer::connectionHandlerThread, this, std::move(connection)).detach();
    }

    DOIP_LOG_INFO("TCP listener thread stopped");
}

/*
 * High-level API: Start the server with automatic connection handling
 */
template <typename Model>
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
        m_workerThreads.emplace_back(&DoIPServer::tcpListenerThread<Model>, this);

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

} // namespace doip

#endif /* DOIPSERVER_H */
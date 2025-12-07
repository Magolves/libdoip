#ifndef DOIPDEFAULTCONNECTION_H
#define DOIPDEFAULTCONNECTION_H

#include "DoIPConfig.h"
#include "DoIPServerModel.h"

#include "DoIPRoutingActivationResult.h"
#include "DoIPTimes.h"
#include "IConnectionContext.h"
#include "TimerManager.h"
#include <optional>

namespace doip {

using namespace std::chrono_literals;

// Timer IDsD& config
enum class ConnectionTimers : uint8_t {
    InitialInactivity,  // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity,  // T_TCP_General_Inactivity (default: 5min)
    AliveCheck,         // T_TCP_Alive_Check (default: 500ms)
    DownstreamResponse, // Server timeout when waiting for a response of the subnet device ("downstream") - not standardized by ISO 13400
    UserDefined         // Placeholder for user-defined timers ()
};

inline std::ostream &operator<<(std::ostream &os, ConnectionTimers tid) {
    switch (tid) {
    case ConnectionTimers::InitialInactivity:
        return os << "Initial Inactivity";
    case ConnectionTimers::GeneralInactivity:
        return os << "General Inactivity";
    case ConnectionTimers::AliveCheck:
        return os << "Alive Check";
    case ConnectionTimers::DownstreamResponse:
        return os << "Downstream Response";
    case ConnectionTimers::UserDefined:
        return os << "User Defined";
    default:
        return os << "Unknown(" << static_cast<int>(tid) << ")";
    }
}

using StateChangeHandler = std::function<void()>;
using MessageHandler = std::function<void(std::optional<DoIPMessage>)>;
using TimeOutHandler = std::function<void(ConnectionTimers)>;

/**
 * @brief Default implementation of IConnectionContext
 *
 * This class provides a default implementation of the IConnectionContext
 * interface, including the state machine and server model. It excludes
 * TCP socket support, making it suitable for non-TCP-based connections.
 */
class DoIPDefaultConnection : public IConnectionContext {

    /**
     * @brief Descriptor for a state in the state machine
     *
     * Contains state, timer, transitions, handlers and timeout duration.
     */
    struct StateDescriptor {
        /**
         * @brief Constructor for StateDescriptor
         * @param state_ The state represented
         * @param timer_ The timer associated with this state
         * @param stateAfterTimeout_ State to transition to after timeout
         * @param handler_ Message handler for this state
         * @param enterState_ Handler called when entering this state
         * @param timeoutHandler_ Handler called on timeout
         * @param timeoutDurationUser_ Custom timeout duration
         */
        StateDescriptor(DoIPServerState state_,
                        DoIPServerState stateAfterTimeout_,
                        MessageHandler handler_,
                        ConnectionTimers timer_ = ConnectionTimers::UserDefined,
                        StateChangeHandler enterState_ = nullptr,
                        TimeOutHandler timeoutHandler_ = nullptr,
                        std::chrono::milliseconds timeoutDurationUser_ = 0ms)
            : state(state_),
              stateAfterTimeout(stateAfterTimeout_),
              messageHandler(std::move(handler_)),
              timer(timer_),
              enterStateHandler(enterState_),
              timeoutHandler(std::move(timeoutHandler_)),
              timeoutDurationUser(timeoutDurationUser_) {}

        DoIPServerState state;                            ///< The state
        DoIPServerState stateAfterTimeout;                ///< State after timeout
        MessageHandler messageHandler;                    ///< Message handler
        ConnectionTimers timer;                           ///< Associated timer
        StateChangeHandler enterStateHandler;             ///< Handler on state entry
        TimeOutHandler timeoutHandler;                    ///< Handler on timeout
        std::chrono::milliseconds timeoutDurationUser{0}; ///< Custom timeout duration
    };

    /**
     * @brief Sets the client address for the connection
     * @param address The client address
     */
    void setClientAddress(const DoIPAddress &address) override;

  public:
    /**
     * @brief Constructs a DoIPDefaultConnection
     * @param model The server model to use
     */
    explicit DoIPDefaultConnection(UniqueServerModelPtr model);

    /**
     * @brief Sends a DoIP protocol message to the client
     * @param msg The message to send
     * @return Number of bytes sent
     */
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override;

    /**
     * @brief Closes the connection
     * @param reason The reason for closure
     */
    void closeConnection(DoIPCloseReason reason) override;

    /**
     * @brief Checks if the connection is open
     * @return true if open, false otherwise
     */
    bool isOpen() const override {
        return m_isOpen;
    }

    /**
     * @brief Gets the reason for connection closure
     * @return The close reason
     */
    DoIPCloseReason getCloseReason() const override {
        return m_closeReason;
    }

    /**
     * @brief Checks if routing is currently activated
     * @return true if routing is activated, false otherwise
     */
    bool isRoutingActivated() const { return m_state && m_state->state == DoIPServerState::RoutingActivated; }

    /**
     * @brief Gets the alive check retry count
     * @return The number of alive check retries
     */
    uint8_t getAliveCheckRetryCount() const { return m_aliveCheckRetryCount; }

    /**
     * @brief Gets the initial inactivity timeout duration
     * @return The initial inactivity timeout in milliseconds
     */
    std::chrono::milliseconds getInitialInactivityTimeout() const { return m_initialInactivityTimeout; }

    /**
     * @brief Gets the general inactivity timeout duration
     * @return The general inactivity timeout in milliseconds
     */
    std::chrono::milliseconds getGeneralInactivityTimeout() const { return m_generalInactivityTimeout; }

    /**
     * @brief Gets the alive check timeout duration
     * @return The alive check timeout in milliseconds
     */
    std::chrono::milliseconds getAliveCheckTimeout() const { return m_aliveCheckTimeout; }

    /**
     * @brief Gets the downstream response timeout duration
     * @return The downstream response timeout in milliseconds
     */
    std::chrono::milliseconds getDownstreamResponseTimeout() const { return m_downstreamResponseTimeout; }

    /**
     * @brief Sets the alive check retry count
     * @param count The number of retries for alive checks
     */
    void setAliveCheckRetryCount(uint8_t count) { m_aliveCheckRetryCount = count; }

    /**
     * @brief Sets the initial inactivity timeout duration
     * @param timeout The timeout duration in milliseconds
     */
    void setInitialInactivityTimeout(std::chrono::milliseconds timeout) { m_initialInactivityTimeout = timeout; }

    /**
     * @brief Sets the general inactivity timeout duration
     * @param timeout The timeout duration in milliseconds
     */
    void setGeneralInactivityTimeout(std::chrono::milliseconds timeout) { m_generalInactivityTimeout = timeout; }

    /**
     * @brief Sets the alive check timeout duration
     * @param timeout The timeout duration in milliseconds
     */
    void setAliveCheckTimeout(std::chrono::milliseconds timeout) { m_aliveCheckTimeout = timeout; }

    /**
     * @brief Sets the downstream response timeout duration
     * @param timeout The timeout duration in milliseconds
     */
    void setDownstreamResponseTimeout(std::chrono::milliseconds timeout) { m_downstreamResponseTimeout = timeout; }

    /**
     * @brief Gets the server's logical address
     * @return The server address
     */
    DoIPAddress getServerAddress() const override;

    /**
     * @brief Gets the client's address
     * @return The client address
     */
    DoIPAddress getClientAddress() const override;

    /**
     * @brief Handles an incoming diagnostic message
     * @param msg The diagnostic message
     * @return Diagnostic acknowledgment (ACK/NACK)
     */
    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override;

    /**
     * @brief Notifies application that connection is closing
     * @param reason The close reason
     */
    void notifyConnectionClosed(DoIPCloseReason reason) override;

    /**
     * @brief Notifies application that diagnostic ACK/NACK was sent
     * @param ack The acknowledgment sent
     */
    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override;

    /**
     * @brief Checks if a downstream handler is present
     * @return true if present, false otherwise
     */
    bool hasDownstreamHandler() const override;

    /**
     * @brief Notifies application of a downstream request
     * @param msg The downstream request message
     * @return Downstream result
     */
    DoIPDownstreamResult notifyDownstreamRequest(const DoIPMessage &msg) override;

    /**
     * @brief Receives a downstream response
     * @param response The downstream response message
     * @param result the downstream result
     */
    void receiveDownstreamResponse(const ByteArray &response, DoIPDownstreamResult result) override;

    /**
     * @brief Gets the current state of the connection
     * @return The current DoIPServerState
     */
    DoIPServerState getState() const {
        return m_state->state;
    }

    /**
     * @brief Gets the server model
     * @return Reference to the server model pointer
     */
    UniqueServerModelPtr &getServerModel() {
        return m_serverModel;
    }

    /**
     * @brief Handles a message (internal helper)
     * @param message The message to handle
     */
    void handleMessage2(const DoIPMessage &message);

  protected:
    UniqueServerModelPtr m_serverModel;
    std::array<StateDescriptor, 7> STATE_DESCRIPTORS;
    DoIPAddress m_routedClientAddress;

    bool m_isOpen;
    DoIPCloseReason m_closeReason = DoIPCloseReason::None;
    const StateDescriptor *m_state = nullptr;
    TimerManager<ConnectionTimers> m_timerManager;

    // Alive check retry (not covered by the standard)
    uint8_t m_aliveCheckRetry{0};
    uint8_t m_aliveCheckRetryCount{DOIP_ALIVE_CHECK_RETRIES};

    // Timer values
    std::chrono::milliseconds m_initialInactivityTimeout{times::server::InitialInactivityTimeout}; // 2 seconds
    std::chrono::milliseconds m_generalInactivityTimeout{times::server::GeneralInactivityTimeout}; // 5 minutes
    std::chrono::milliseconds m_aliveCheckTimeout{times::server::AliveCheckResponseTimeout};       // 500 ms
    std::chrono::milliseconds m_downstreamResponseTimeout{10s};                                    // 500 ms

    // State transition
    void transitionTo(DoIPServerState newState);

    // State timer
    std::chrono::milliseconds getTimerDuration(StateDescriptor const *stateDesc);

    void startStateTimer(StateDescriptor const *stateDesc);
    void restartStateTimer();

    // handlers for each state
    void handleSocketInitialized(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitRoutingActivation(DoIPServerEvent event, OptDoIPMessage msg);
    void handleRoutingActivated(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitAliveCheckResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitDownstreamResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleFinalize(DoIPServerEvent event, OptDoIPMessage msg);

    /**
     * @brief Default timeout handler
     *
     * @param timer_id the timer ID that expired
     */
    void handleTimeout(ConnectionTimers timer_id);

    ssize_t sendRoutingActivationResponse(const DoIPAddress &source_address, DoIPRoutingActivationResult response_code);
    ssize_t sendAliveCheckRequest();
    ssize_t sendDiagnosticMessageResponse(const DoIPAddress &sourceAddress, DoIPDiagnosticAck ack);
    ssize_t sendDownstreamResponse(const DoIPAddress &sourceAddress, const ByteArray& payload);
};

} // namespace doip

#endif /* DOIPDEFAULTCONNECTION_H */
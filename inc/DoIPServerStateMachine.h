#ifndef DOIPSERVERSTATEMACHINE_H
#define DOIPSERVERSTATEMACHINE_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>

#include "DoIPTimes.h"
#include "TimerManager.h"
#include "DoIPIdentifiers.h"
#include "DoIPIdentifiers.h"

namespace doip {

// Forward declarations
class DoIPAddress;

// DoIP Protocol States
// Figure 26
enum class DoIPState {
    SocketInitialized,      // Initial state after socket creation
    WaitRoutingActivation,  // Waiting for routing activation request
    RoutingActivated,       // Routing is active, ready for diagnostics
    WaitAliveCheckResponse, // D& config  Waiting for alive check response
    Finalize,               // Cleanup state
    Closed                  // Connection closed
};

// Stream operator for DoIPState
inline std::ostream& operator<<(std::ostream& os, DoIPState state) {
    switch (state) {
        case DoIPState::SocketInitialized:      return os << "SocketInitialized";
        case DoIPState::WaitRoutingActivation:  return os << "WaitRoutingActivation";
        case DoIPState::RoutingActivated:       return os << "RoutingActivated";
        case DoIPState::WaitAliveCheckResponse: return os << "WaitAliveCheckResponse";
        case DoIPState::Finalize:               return os << "Finalize";
        case DoIPState::Closed:                 return os << "Closed";
        default:                                return os << "Unknown(" << static_cast<int>(state) << ")";
    }
}

// DoIP Events
enum class DoIPEvent {
    // Message events
    RoutingActivationReceived,
    AliveCheckResponseReceived,
    DiagnosticMessageReceived,
    CloseRequestReceived,

    // Timer events
    Initial_inactivity_timeout,
    GeneralInactivityTimeout,
    AliveCheckTimeout,

    // Error events
    InvalidMessage,
    SocketError
};

// Stream operator for DoIPEvent
inline std::ostream& operator<<(std::ostream& os, DoIPEvent event) {
    switch (event) {
        case DoIPEvent::RoutingActivationReceived:   return os << "RoutingActivationReceived";
        case DoIPEvent::AliveCheckResponseReceived:  return os << "AliveCheckResponseReceived";
        case DoIPEvent::DiagnosticMessageReceived:   return os << "DiagnosticMessageReceived";
        case DoIPEvent::CloseRequestReceived:        return os << "CloseRequestReceived";
        case DoIPEvent::Initial_inactivity_timeout:  return os << "Initial_inactivity_timeout";
        case DoIPEvent::GeneralInactivityTimeout:  return os << "GeneralInactivityTimeout";
        case DoIPEvent::AliveCheckTimeout:         return os << "AliveCheckTimeout";
        case DoIPEvent::InvalidMessage:              return os << "InvalidMessage";
        case DoIPEvent::SocketError:                 return os << "SocketError";
        default:                                     return os << "Unknown(" << static_cast<int>(event) << ")";
    }
}

// Timer IDsD& config
enum class TimerID {
    InitialInactivity, // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity, // T_TCP_General_Inactivity (default: 5min)
    AliveCheck         // T_TCP_Alive_Check (default: 500ms)
};

inline std::ostream& operator<<(std::ostream& os, TimerID tid) {
    switch(tid) {
        case TimerID::InitialInactivity: return os << "Initial Inactivity";
        case TimerID::GeneralInactivity: return os << "General Inactivity";
        case TimerID::AliveCheck: return os << "Alive Check";
        default: return os << "Unknown(" << static_cast<int>(tid) << ")";
    }
}

// Forward declarations
class DoIPMessage;
class Timer;

/**
 * @brief DoIP Server State Machine implementing ISO 13400-2:2019 protocol
 *
 * This class manages the state transitions and message processing for a DoIP server connection.
 * It handles routing activation, alive checks, diagnostic messages, and timeout management.
 */
class DoIPServerStateMachine {
  public:
    using StateHandler = std::function<void(DoIPEvent, const DoIPMessage *)>;
    using TransitionHandler = std::function<void(DoIPState, DoIPState)>;
    using CloseConnectionHandler = std::function<void()>;
    using SendMessageHandler = std::function<void(const DoIPMessage &)>;

    /**
     * @brief Constructs a new DoIP Server State Machine
     * @param onClose Callback function to be invoked when the connection should be closed
     */
    explicit DoIPServerStateMachine(CloseConnectionHandler onClose) : m_closeConnectionCallback(onClose) {
        transitionTo(DoIPState::WaitRoutingActivation);
        m_timerManager.addTimer(std::chrono::milliseconds(times::server::InitialInactivityTimeout), onClose);
    }

    /**
     * @brief Destroys the DoIP Server State Machine
     */
    ~DoIPServerStateMachine();

    // Main interface

    /**
     * @brief Processes an incoming DoIP message
     * @param message The DoIP message to process
     */
    void processMessage(const DoIPMessage &message);

    /**
     * @brief Processes a DoIP event
     * @param event The event to process
     */
    void processEvent(DoIPEvent event);

    /**
     * @brief Handles a timer timeout event
     * @param timer_id The ID of the timer that expired
     */
    void handleTimeout(TimerID timer_id);

    // State queries

    /**
     * @brief Gets the current state of the state machine
     * @return The current DoIPState
     */
    DoIPState getState() const { return m_state; }

    /**
     * @brief Checks if routing is currently activated
     * @return true if routing is activated, false otherwise
     */
    bool isRoutingActivated() const { return m_state == DoIPState::RoutingActivated; }

    // Configuration

    /**
     * @brief Sets the callback for state transitions
     * @param callback Function to be called on state transitions with (old_state, new_state)
     */
    void setTransitionCallback(TransitionHandler callback) {
        m_transitionCallback = callback;
    }

    /**
     * @brief Sets the callback for sending messages
     * @param callback Function to be called when a message needs to be sent
     */
    void setSendMessageCallback(SendMessageHandler callback) {
        m_sendMessageCallback = callback;
    }

    /**
     * @brief Gets the active source address
     * @return The currently active source address
     */
    uint16_t getActiveSourceAddress() const { return m_activeSourceAddress; }

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

    // Runtime data setters

    /**
     * @brief Sets the active source address
     * @param address The source address to set as active
     */
    void setActiveSourceAddress(uint16_t address) { m_activeSourceAddress = address; }

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

  private:
    // State handlers
    void handleSocketInitialized(DoIPEvent event, const DoIPMessage *msg);
    void handleWaitRoutingActivation(DoIPEvent event, const DoIPMessage *msg);
    void handleRoutingActivated(DoIPEvent event, const DoIPMessage *msg);
    void handleWaitAliveCheckResponse(DoIPEvent event, const DoIPMessage *msg);
    void handleFinalize(DoIPEvent event, const DoIPMessage *msg);

    // Helper methods
    void transitionTo(DoIPState new_state);
    void startTimer(TimerID timer_id, std::chrono::milliseconds duration);
    void stopTimer(TimerID timer_id);
    void stopAllTimers();
    void sendRoutingActivationResponse(DoIPAddress sourceAddress, uint8_t response_code);
    void sendAliveCheckRequest();
    void sendDiagnosticMessageAck(DoIPAddress sourceAddress);
    void sendDiagnosticMessageNack(DoIPAddress sourceAddress, uint8_t nack_code);

    // Helper function to forward messages
    void notifyMessage(const DoIPMessage &msg);

    // Inline helper functions for common timer operations
    inline void startGeneralInactivityTimer() {
        startTimer(TimerID::GeneralInactivity, m_generalInactivityTimeout);
    }

    inline void startAliveCheckTimer() {
        startTimer(TimerID::AliveCheck, m_aliveCheckTimeout);
    }

    TimerManager m_timerManager;

    // State
    DoIPState m_state;

    // Callbacks
    CloseConnectionHandler m_closeConnectionCallback;
    TransitionHandler m_transitionCallback;
    SendMessageHandler m_sendMessageCallback;

    // Runtime data
    uint16_t m_activeSourceAddress;
    uint8_t m_aliveCheckRetryCount;
    std::chrono::milliseconds m_initialInactivityTimeout{times::server::InitialInactivityTimeout}; // 2 seconds
    std::chrono::milliseconds m_generalInactivityTimeout{times::server::GeneralInactivityTimeout}; // 5 minutes
    std::chrono::milliseconds m_aliveCheckTimeout{times::server::AliveCheckResponseTimeout};       // 500 ms
};

} // namespace doip


#endif  /* DOIPSERVERSTATEMACHINE_H */

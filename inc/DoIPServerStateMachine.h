#ifndef DOIPSERVERSTATEMACHINE_H
#define DOIPSERVERSTATEMACHINE_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>

#include "DoIPEvent.h"
#include "DoIPIdentifiers.h"
#include "DoIPState.h"
#include "DoIPTimes.h"
#include "IConnectionContext.h"
#include "DoIPRoutingActivationResult.h"
#include "TimerManager.h"

namespace doip {

// Forward declarations
struct DoIPAddress;
class DoIPMessage;

// Timer IDsD& config
enum class StateMachineTimer : uint8_t {
    InitialInactivity, // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity, // T_TCP_General_Inactivity (default: 5min)
    AliveCheck         // T_TCP_Alive_Check (default: 500ms)
};

inline std::ostream &operator<<(std::ostream &os, StateMachineTimer tid) {
    switch (tid) {
    case StateMachineTimer::InitialInactivity:
        return os << "Initial Inactivity";
    case StateMachineTimer::GeneralInactivity:
        return os << "General Inactivity";
    case StateMachineTimer::AliveCheck:
        return os << "Alive Check";
    default:
        return os << "Unknown(" << static_cast<int>(tid) << ")";
    }
}

/**
 * @brief DoIP Server State Machine implementing ISO 13400-2:2019 protocol
 *
 * This class manages the state transitions and message processing for a DoIP server connection.
 * It handles routing activation, alive checks, diagnostic messages, and timeout management.
 */
class DoIPServerStateMachine {
  public:
    using TransitionHandler = std::function<void(DoIPState, DoIPState)>;

    /**
     * @brief Constructs a new DoIP Server State Machine
     * @param context Reference to the connection context interface
     */
    explicit DoIPServerStateMachine(IConnectionContext &context)
        : m_context(context),
          m_state(DoIPState::SocketInitialized),
          m_aliveCheckRetryCount(0) {
        transitionTo(DoIPState::WaitRoutingActivation);
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
     * @param msg The associated message, if any
     */
    void processEvent(DoIPEvent event, OptDoIPMessage msg = std::nullopt);

    /**
     * @brief Handles a timer timeout event
     * @param timer_id The ID of the timer that expired
     */
    void handleTimeout(StateMachineTimer timer_id);

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
     * @brief Sets the callback for state transitions (optional, for debugging/logging)
     * @param callback Function to be called on state transitions with (old_state, new_state)
     */
    void setTransitionCallback(TransitionHandler callback) {
        m_transitionCallback = callback;
    }

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
    void handleSocketInitialized(DoIPEvent event, OptDoIPMessage msg);
    void handleWaitRoutingActivation(DoIPEvent event, OptDoIPMessage msg);
    void handleRoutingActivated(DoIPEvent event, OptDoIPMessage msg);
    void handleWaitAliveCheckResponse(DoIPEvent event, OptDoIPMessage msg);
    void handleFinalize(DoIPEvent event, OptDoIPMessage msg);

    /**
     * @brief Handles common transitions valid for all states
     *
     * @param event the event to process
     * @param msg the associated message, if any
     */
    bool  handleCommonTransition(DoIPEvent event, OptDoIPMessage msg);

    // Helper methods
    void transitionTo(DoIPState new_state, DoIPCloseReason reason = DoIPCloseReason::None);
    void startTimer(StateMachineTimer timer_id, std::chrono::milliseconds duration);
    void stopTimer(StateMachineTimer timer_id);
    void stopAllTimers();
    ssize_t sendRoutingActivationResponse(const DoIPAddress &sourceAddress, DoIPRoutingActivationResult response_code);
    ssize_t sendAliveCheckRequest();

    ssize_t sendDiagnosticMessageResponse(const DoIPAddress &sourceAddress, DoIPDiagnosticAck ack);
    ssize_t sendDiagnosticMessageAck(const DoIPAddress &sourceAddress);
    ssize_t sendDiagnosticMessageNack(const DoIPAddress &sourceAddress, DoIPNegativeDiagnosticAck nack);

    // Helper function to forward messages
    ssize_t sendMessage(const DoIPMessage &msg);

    // Inline helper functions for common timer operations
    inline void startGeneralInactivityTimer() {
        startTimer(StateMachineTimer::GeneralInactivity, m_generalInactivityTimeout);
    }

    inline void startAliveCheckTimer() {
        startTimer(StateMachineTimer::AliveCheck, m_aliveCheckTimeout);
    }

    // Interface to connection layer
    IConnectionContext &m_context;

    TimerManager<StateMachineTimer> m_timerManager;

    // State
    DoIPState m_state;

    // Optional callback for state transitions (debugging/logging)
    TransitionHandler m_transitionCallback;

    // Runtime data
    uint8_t m_aliveCheckRetryCount;
    std::chrono::milliseconds m_initialInactivityTimeout{times::server::InitialInactivityTimeout}; // 2 seconds
    std::chrono::milliseconds m_generalInactivityTimeout{times::server::GeneralInactivityTimeout}; // 5 minutes
    std::chrono::milliseconds m_aliveCheckTimeout{times::server::AliveCheckResponseTimeout};       // 500 ms
};





} // namespace doip

#endif /* DOIPSERVERSTATEMACHINE_H */

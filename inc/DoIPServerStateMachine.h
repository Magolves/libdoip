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
    General_inactivity_timeout,
    Alive_check_timeout,

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
        case DoIPEvent::General_inactivity_timeout:  return os << "General_inactivity_timeout";
        case DoIPEvent::Alive_check_timeout:         return os << "Alive_check_timeout";
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

// Configuration values (ISO 13400-2 default values)
struct DoIPConfig {
    std::chrono::milliseconds initialInactivityTimeout{times::server::InitialInactivityTimeout}; // 2 seconds
    std::chrono::milliseconds generalInactivityTimeout{times::server::GeneralInactivityTimeout}; // 5 minutes
    std::chrono::milliseconds aliveCheckTimeout{times::server::AliveCheckResponseTimeout};       // 500 ms
    uint8_t max_alive_check_retries{times::server::VehicleAnnouncementNumber};
    uint16_t logical_address{0x0E80}; // Server logical address
};

// Forward declarations
class DoIPMessage;
class Timer;

// State machine class
class DoIPServerStateMachine {
  public:
    using StateHandler = std::function<void(DoIPEvent, const DoIPMessage *)>;
    using TransitionHandler = std::function<void(DoIPState, DoIPState)>;
    using CloseConnectionHandler = std::function<void()>;
    using SendMessageCallback = std::function<void(const DoIPMessage &)>;

    explicit DoIPServerStateMachine(CloseConnectionHandler onClose) : m_closeConnectionCallback(onClose) {
        transitionTo(DoIPState::WaitRoutingActivation);
        m_timerManager.addTimer(std::chrono::milliseconds(times::server::InitialInactivityTimeout), onClose);
    }
    ~DoIPServerStateMachine();

    // Main interface
    void processMessage(const DoIPMessage &message);
    void processEvent(DoIPEvent event);
    void handleTimeout(TimerID timer_id);

    // State queries
    DoIPState getState() const { return m_state; }
    bool isRoutingActivated() const { return m_state == DoIPState::RoutingActivated; }

    // Configuration
    void setTransitionCallback(TransitionHandler callback) {
        m_transitionCallback = callback;
    }

    // Message sending callback (to be implemented by user)

    void setSendMessageCallback(SendMessageCallback callback) {
        m_sendMessageCallback = callback;
    }

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

    TimerManager m_timerManager;

    // State
    DoIPState m_state;

    // Callbacks
    CloseConnectionHandler m_closeConnectionCallback;
    TransitionHandler m_transitionCallback;
    SendMessageCallback m_sendMessageCallback;

    // Runtime data
    uint16_t m_activeSourceAddress;
    uint8_t m_aliveCheckRetryCount;
};

} // namespace doip


#endif  /* DOIPSERVERSTATEMACHINE_H */

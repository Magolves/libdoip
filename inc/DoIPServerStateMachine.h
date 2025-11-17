#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <queue>

#include "DoIPTimes.h"
#include "TimerManager.h"
#include "DoIPIdentifiers.h"

namespace doip {

// Forward declarations
class DoIPAddress;

// Callback type for close connection
using CloseConnectionCallback = std::function<void()>;

// DoIP Protocol States
enum class DoIPState {
    SocketInitialized,      // Initial state after socket creation
    WaitRoutingActivation,  // Waiting for routing activation request
    RoutingActivated,       // Routing is active, ready for diagnostics
    WaitAliveCheckResponse, // D& config  Waiting for alive check response
    Finalize,               // Cleanup state
    Closed                  // Connection closed
};

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
    Invalid_message,
    Socket_error
};

// Timer IDsD& config
enum class TimerID {
    InitialInactivity, // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity, // T_TCP_General_Inactivity (default: 5min)
    AliveCheck         // T_TCP_Alive_Check (default: 500ms)
};

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
    using TransitionCallback = std::function<void(DoIPState, DoIPState)>;

    explicit DoIPServerStateMachine(CloseConnectionCallback onClose, TimerManager &timer = TimerManager::getInstance()) : m_timerManager(timer), m_state(DoIPState::SocketInitialized), m_closeConnectionCallback(onClose) {
        m_timerManager.addTimer(std::chrono::milliseconds(times::server::InitialInactivityTimeout), onClose);
    }
    ~DoIPServerStateMachine();

    // Main interface
    void processMessage(const DoIPMessage &message);
    void processEvent(DoIPEvent event);
    void handleTimeout(TimerID timer_id);

    // State queries
    DoIPState getCurrentState() const { return m_state; }
    bool isRoutingActivated() const { return m_state == DoIPState::RoutingActivated; }

    // Configuration
    void setTransitionCallback(TransitionCallback callback) {
        m_transitionCallback = callback;
    }

    // Message sending callback (to be implemented by user)
    using SendMessageCallback = std::function<void(const DoIPMessage &)>;
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

    TimerManager &m_timerManager;

    // State
    DoIPState m_state;

    // Callbacks
    CloseConnectionCallback m_closeConnectionCallback;
    TransitionCallback m_transitionCallback;
    SendMessageCallback m_sendMessageCallback;

    // Runtime data
    uint16_t m_activeSourceAddress;
    uint8_t m_aliveCheckRetryCount;
};

} // namespace doip

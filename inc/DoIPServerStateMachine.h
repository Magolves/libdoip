#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <chrono>
#include <map>

#include "DoIPTimes.h"
#include "DoIPServer.h"

namespace doip {

// DoIP Protocol States
enum class DoIPState {
    Socket_initialized,           // Initial state after socket creation
    Wait_routing_activation,      // Waiting for routing activation request
    Routing_activated,            // Routing is active, ready for diagnostics
    Wait_alive_check_response,    //D& config  Waiting for alive check response
    Finalize,                     // Cleanup state
    Closed                        // Connection closed
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
    InitialInactivity,    // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity,    // T_TCP_General_Inactivity (default: 5min)
    AliveCheck            // T_TCP_Alive_Check (default: 500ms)
};

// Configuration values (ISO 13400-2 default values)
struct DoIPConfig {
    std::chrono::milliseconds initialInactivityTimeout{times::server::InitialInactivityTimeout};    // 2 seconds
    std::chrono::milliseconds generalInactivityTimeout{times::server::GeneralInactivityTimeout};  // 5 minutes
    std::chrono::milliseconds aliveCheckTimeout{times::server::AliveCheckResponseTimeout};            // 500 ms
    uint8_t max_alive_check_retries{times::server::VehicleAnnouncementNumber};
    uint16_t logical_address{0x0E80};  // Server logical address
};

// Forward declarations
class DoIPMessage;
class Timer;

// State machine class
class DoIPServerStateMachine {
public:
    using StateHandler = std::function<void(DoIPEvent, const DoIPMessage*)>;
    using TransitionCallback = std::function<void(DoIPState, DoIPState)>;

    explicit DoIPServerStateMachine(const DoIPServer& server);
    ~DoIPServerStateMachine();

    // Main interface
    void processMessage(const DoIPMessage& message);
    void processEvent(DoIPEvent event);
    void handleTimeout(TimerID timer_id);

    // State queries
    DoIPState getCurrentState() const { return m_state; }
    bool isRoutingActivated() const { return m_state == DoIPState::Routing_activated; }

    // Configuration
    void setConfig(const DoIPServer& server) { config_ = config; }
    void setTransitionCallback(TransitionCallback callback) {
        m_transition_callback = callback;
    }

    // Message sending callback (to be implemented by user)
    using SendMessageCallback = std::function<void(const DoIPMessage&)>;
    void setSendMessageCallback(SendMessageCallback callback) {
        send_message_callback_ = callback;
    }

private:
    // State handlers
    void handleSocketInitialized(DoIPEvent event, const DoIPMessage* msg);
    void handleWaitRoutingActivation(DoIPEvent event, const DoIPMessage* msg);
    void handleRoutingActivated(DoIPEvent event, const DoIPMessage* msg);
    void handleWaitAliveCheckResponse(DoIPEvent event, const DoIPMessage* msg);
    void handleFinalize(DoIPEvent event, const DoIPMessage* msg);

    // Helper methods
    void transitionTo(DoIPState new_state);
    void startTimer(TimerID timer_id, std::chrono::milliseconds duration);
    void stopTimer(TimerID timer_id);
    void stopAllTimers();
    void sendRoutingActivationResponse(uint16_t source_address, uint8_t response_code);
    void sendAliveCheckRequest();
    void sendDiagnosticMessageAck(uint16_t source_address);
    void sendDiagnosticMessageNack(uint16_t source_address, uint8_t nack_code);

    // State and configuration
    DoIPState m_state;
    DoIPServer& m_server;

    // Callbacks
    TransitionCallback m_transition_callback;
    SendMessageCallback send_message_callback_;

    // Runtime data
    uint16_t active_source_address_;
    uint8_t alive_check_retry_count_;
    std::map<TimerID, std::unique_ptr<Timer>> timers_;
};


} // namespace doip

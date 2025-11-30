#ifndef DOIPDEFAULTCONNECTION_H
#define DOIPDEFAULTCONNECTION_H

#include "DoIPServerModel.h"
#include "DoIPServerStateMachine.h"
#include "IConnectionContext.h"
#include <optional>

namespace doip {

using namespace std::chrono_literals;

// Timer IDsD& config
enum class ConnectionTimers : uint8_t {
    InitialInactivity,  // T_TCP_Initial_Inactivity (default: 2s)
    GeneralInactivity,  // T_TCP_General_Inactivity (default: 5min)
    AliveCheck,         // T_TCP_Alive_Check (default: 500ms)
    DownstreamResponse, // Server timeout when waiting for a response of the subnet device ("downstream") - not standardized by ISO 13400
    UserDefined         // Placeholder for user-defined timers
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


using MessageHandler = std::function<void(std::optional<DoIPMessage>)>;
using TimeOutHandler = std::function<void(StateMachineTimer)>;

constexpr std::array<std::pair<ConnectionTimers, std::chrono::milliseconds>, 5> DEFAULT_TIMER_DURATIONS{{{ConnectionTimers::InitialInactivity, doip::times::server::InitialInactivityTimeout},
                                                                                                         {ConnectionTimers::GeneralInactivity, doip::times::server::GeneralInactivityTimeout},
                                                                                                         {ConnectionTimers::AliveCheck, doip::times::server::AliveCheckResponseTimeout},
                                                                                                         {ConnectionTimers::DownstreamResponse, 2000ms},
                                                                                                         {ConnectionTimers::UserDefined, 0ms}}};

/**
 * @brief Default implementation of IConnectionContext
 *
 * This class provides a default implementation of the IConnectionContext
 * interface, including the state machine and server model. It excludes
 * TCP socket support, making it suitable for non-TCP-based connections.
 */
class DoIPDefaultConnection : public IConnectionContext {
    struct StateDescriptor {
        StateDescriptor(DoIPServerState state_,
                        ConnectionTimers timer_,
                        DoIPServerState stateAfterTimeout_,
                        MessageHandler handler_,
                        TimeOutHandler timeoutHandler_ = nullptr,
                        std::chrono::milliseconds timeoutDurationUser_ = std::chrono::milliseconds(0))
            : state(state_),
              timer(timer_),
              stateAfterTimeout(stateAfterTimeout_),
              messageHandler(std::move(handler_)),
              timeoutHandler(std::move(timeoutHandler_)),
              timeoutDurationUser(timeoutDurationUser_) {}

        DoIPServerState state;
        ConnectionTimers timer;
        DoIPServerState stateAfterTimeout;

        MessageHandler messageHandler;
        TimeOutHandler timeoutHandler;
        std::chrono::milliseconds timeoutDurationUser{0};
    };

  public:
    explicit DoIPDefaultConnection(UniqueServerModelPtr model);

    // === IConnectionContext interface implementation ===
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override;
    void closeConnection(DoIPCloseReason reason) override;

    bool isOpen() const override {
        return m_isOpen;
    }

    DoIPCloseReason getCloseReason() const override {
        return m_closeReason;
    }

    DoIPAddress getServerAddress() const override;
    DoIPAddress getClientAddress() const override;
    void setClientAddress(const DoIPAddress &address) override;
    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override;
    void notifyConnectionClosed(DoIPCloseReason reason) override;
    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override;
    bool hasDownstreamHandler() const override;
    DoIPDownstreamResult notifyDownstreamRequest(const DoIPMessage &msg) override;
    void receiveDownstreamResponse(const DoIPMessage &response) override;
    void notifyDownstreamResponseReceived(const DoIPMessage &request, const DoIPMessage &response) override;

    DoIPServerState getState() const {
        return m_state->state;
    }

    UniqueServerModelPtr &getServerModel() {
        return m_serverModel;
    }

    void handleMessage2(const DoIPMessage &message);

  protected:
    UniqueServerModelPtr m_serverModel;
    std::array<StateDescriptor, 7> STATE_DESCRIPTORS;
    DoIPAddress m_routedClientAddress;

    bool m_isOpen;
    DoIPCloseReason m_closeReason = DoIPCloseReason::None;
    const StateDescriptor *m_state = nullptr;
    std::array<std::pair<ConnectionTimers, std::chrono::milliseconds>, 5> m_timerDurations = DEFAULT_TIMER_DURATIONS;
    TimerManager<ConnectionTimers> m_timerManager;

    void transitionTo(DoIPServerState newState);

    void startStateTimer(StateDescriptor const *stateDesc) {
        assert(stateDesc != nullptr);
        if (stateDesc == nullptr) {
            return;
        }

        std::chrono::milliseconds duration = std::chrono::milliseconds(0);
        auto timerDesc = std::find_if(
            m_timerDurations.begin(),
            m_timerDurations.end(),
            [stateDesc](const auto &pair) { return pair.first == stateDesc->timer; });

        DOIP_LOG_DEBUG("Starting timer for state {}: Timer ID {}", fmt::streamed(stateDesc->state), fmt::streamed(stateDesc->timer));

        if (stateDesc->timer == ConnectionTimers::UserDefined) {
            duration = stateDesc->timeoutDurationUser;
            // If user-defined timer duration is zero, transition immediately
            if (duration.count() == 0) {
                DOIP_LOG_DEBUG("User-defined timer duration is zero, transitioning immediately to state {}", fmt::streamed(stateDesc->stateAfterTimeout));
                transitionTo(stateDesc->stateAfterTimeout);
                return;
            }
        }

        TimeOutHandler timeoutHandler = stateDesc->timeoutHandler;
        if (timeoutHandler == nullptr) {
            if (stateDesc->timeoutDurationUser.count() > 0) {
                auto id = m_timerManager.addTimer(timerDesc->first, duration, [this](ConnectionTimers timerId) {
                    handleTimeout(timerId);
                }, false);

                if (id.has_value()) {
                    DOIP_LOG_DEBUG("Started user-defined timer {} for {}ms", fmt::streamed(timerDesc->first), duration.count());
                } else {
                    DOIP_LOG_ERROR("Failed to start user-defined timer {}", fmt::streamed(timerDesc->first));
                }
            }
        }
    }

    // handlers for each state
    void handleSocketInitialized(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitRoutingActivation(DoIPServerEvent event, OptDoIPMessage msg);
    void handleRoutingActivated(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitAliveCheckResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitDownstreamResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleFinalize(DoIPServerEvent event, OptDoIPMessage msg);

    // timeout handlers for each state
    void handleTimeout(ConnectionTimers timer_id);

    // void handleInitialInactivityTimeout();
    // void handleGeneralInactivityTimeout();
    // void handleAliveCheckTimeout();
    // void handleDownstreamTimeout();
};

} // namespace doip

#endif /* DOIPDEFAULTCONNECTION_H */
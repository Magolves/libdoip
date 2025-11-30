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

using MessageHandler = std::function<void(std::optional<DoIPMessage>)>;
using TimeOutHandler = std::function<void(StateMachineTimer)>;
/**
 * @brief Default implementation of IConnectionContext
 *
 * This class provides a default implementation of the IConnectionContext
 * interface, including the state machine and server model. It excludes
 * TCP socket support, making it suitable for non-TCP-based connections.
 */
class DoIPDefaultConnection : public IConnectionContext {
    struct StateDescriptor {
        StateDescriptor(DoIPServerState state,
                        ConnectionTimers timer,
                        DoIPServerState stateAfterTimeout,
                        MessageHandler handler,
                        TimeOutHandler timeoutHandler = nullptr,
                        std::chrono::milliseconds timeoutDurationUser = std::chrono::milliseconds(0))
            : state(state),
              timer(timer),
              stateAfterTimeout(stateAfterTimeout),
              messageHandler(std::move(handler)),
              timeoutHandler(std::move(timeoutHandler)),
              timeoutDurationUser(timeoutDurationUser) {}

        DoIPServerState state;
        ConnectionTimers timer;
        DoIPServerState stateAfterTimeout;
        std::chrono::milliseconds timeoutDurationUser{0};

        MessageHandler messageHandler;
        TimeOutHandler timeoutHandler;
    };

  public:
    explicit DoIPDefaultConnection(UniqueServerModelPtr model);

    std::array<StateDescriptor, 7> STATE_DESCRIPTORS;

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
        return m_stateMachine.getState();
    }

    UniqueServerModelPtr &getServerModel() {
        return m_serverModel;
    }

    void handleMessage(const DoIPMessage &message);

  protected:
    DoIPServerStateMachine m_stateMachine;
    UniqueServerModelPtr m_serverModel;
    DoIPAddress m_routedClientAddress;

    bool m_isOpen;
    DoIPCloseReason m_closeReason = DoIPCloseReason::None;
    const StateDescriptor *m_state = nullptr;
    TimerManager<ConnectionTimers> m_timerManager;

    void transitionTo(DoIPServerState newState, DoIPCloseReason reason = DoIPCloseReason::None);

    // handlers for each state
    void handleSocketInitialized(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitRoutingActivation(DoIPServerEvent event, OptDoIPMessage msg);
    void handleRoutingActivated(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitAliveCheckResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleWaitDownstreamResponse(DoIPServerEvent event, OptDoIPMessage msg);
    void handleFinalize(DoIPServerEvent event, OptDoIPMessage msg);

    // timeout handlers for each state
    void handleInitialInactivityTimeout();
    void handleGeneralInactivityTimeout();
    void handleAliveCheckTimeout();
    void handleDownstreamTimeout();
};

} // namespace doip

#endif /* DOIPDEFAULTCONNECTION_H */
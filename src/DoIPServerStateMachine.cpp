#include "DoIPServerStateMachine.h"
#include "DoIPConfig.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"

namespace doip {

DoIPServerStateMachine::~DoIPServerStateMachine() {
    stopAllTimers();
}

void DoIPServerStateMachine::processMessage(const DoIPMessage &message) {
    // Map message type to event
    DoIPEvent event;
    DOIP_LOG_INFO("processMessage of type {}", fmt::streamed(message.getPayloadType()));

    switch (message.getPayloadType()) {
    case DoIPPayloadType::RoutingActivationRequest:
        event = DoIPEvent::RoutingActivationReceived;
        break;
    case DoIPPayloadType::AliveCheckResponse:
        event = DoIPEvent::AliveCheckResponseReceived;
        break;
    case DoIPPayloadType::DiagnosticMessage:
        event = DoIPEvent::DiagnosticMessageReceived;
        break;
    default:
        event = DoIPEvent::InvalidMessage;
        break;
    }

    processEvent(event, message);
}

void DoIPServerStateMachine::processEvent(DoIPEvent event, OptDoIPMessage msg) {
    DOIP_LOG_INFO("processEvent {} in state {}", fmt::streamed(event), fmt::streamed(m_state));

    switch (event) {
    case DoIPEvent::AliveCheckTimeout:
    case DoIPEvent::Initial_inactivity_timeout:
    case DoIPEvent::GeneralInactivityTimeout:
        transitionTo(DoIPState::Finalize);
        break;
    default:
        break;
    }

    // Process events based on current state
    switch (m_state) {
    case DoIPState::SocketInitialized:
        handleSocketInitialized(event, msg);
        break;
    case DoIPState::WaitRoutingActivation:
        handleWaitRoutingActivation(event, msg);
        break;
    case DoIPState::RoutingActivated:
        handleRoutingActivated(event, msg);
        break;
    case DoIPState::WaitAliveCheckResponse:
        handleWaitAliveCheckResponse(event, msg);
        break;
    case DoIPState::Finalize:
        handleFinalize(event, msg);
        break;
    case DoIPState::Closed:
        // No processing in closed state
        break;
    }
}

void DoIPServerStateMachine::handleTimeout(StateMachineTimer timer_id) {
    DoIPEvent event = DoIPEvent::SocketError; // Default value

    DOIP_LOG_WARN("Timeout '{}'", fmt::streamed(timer_id));

    switch (timer_id) {
    case StateMachineTimer::InitialInactivity:
        event = DoIPEvent::Initial_inactivity_timeout;
        break;
    case StateMachineTimer::GeneralInactivity:
        event = DoIPEvent::GeneralInactivityTimeout;
        break;
    case StateMachineTimer::AliveCheck:
        event = DoIPEvent::AliveCheckTimeout;
        break;
    }

    processEvent(event);
}

// State Handlers

void DoIPServerStateMachine::handleSocketInitialized(DoIPEvent event, OptDoIPMessage msg) {
    if (handleCommonTransition(event, msg)) {
        return;
    }

    transitionTo(DoIPState::WaitRoutingActivation);

    DOIP_LOG_INFO("Socket init {} in state {}", fmt::streamed(event), fmt::streamed(m_state));
}

void DoIPServerStateMachine::handleWaitRoutingActivation(DoIPEvent event, OptDoIPMessage msg) {
    if (handleCommonTransition(event, msg)) {
        return;
    }

    if (event == DoIPEvent::RoutingActivationReceived) {
        if (!msg) {
            transitionTo(DoIPState::Finalize, DoIPCloseReason::SocketError);
            return;
        }

        auto sourceAddress = msg->getSourceAddress();
        bool hasAddress = sourceAddress.has_value();
        bool rightPayloadType = (msg->getPayloadType() == DoIPPayloadType::RoutingActivationRequest);

        if (!hasAddress || !rightPayloadType) {
            DOIP_LOG_WARN("Invalid Routing Activation Request message");
            transitionTo(DoIPState::Finalize, DoIPCloseReason::InvalidMessage);
            return;
        }

        // Set client address in context
        m_context.setClientAddress(sourceAddress.value());

        // Send Routing Activation Response
        sendRoutingActivationResponse(sourceAddress.value(), DoIPRoutingActivationResult::RouteActivated);

        // Stop initial inactivity timer, start general inactivity timer
        stopTimer(StateMachineTimer::InitialInactivity);
        startGeneralInactivityTimer();

        // Transition to Routing Activated state
        transitionTo(DoIPState::RoutingActivated);
        return;
    }
    DOIP_LOG_WARN("handleWaitRoutingActivation: Unexpected event {} in state {}", fmt::streamed(event), fmt::streamed(m_state));
}

// Routing Activated State: In this state we are waiting for diagnostic messages
void DoIPServerStateMachine::handleRoutingActivated(DoIPEvent event, OptDoIPMessage msg) {
    if (handleCommonTransition(event, msg)) {
        return;
    }

    if (event == DoIPEvent::DiagnosticMessageReceived) {
        if (msg) {
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                auto ack = m_context.notifyDiagnosticMessage(*msg);
                sendDiagnosticMessageResponse(sourceAddress.value(), ack);

                // Reset general inactivity timer
                stopTimer(StateMachineTimer::GeneralInactivity);
                startGeneralInactivityTimer();
            }
        }
        return;
    }

    DOIP_LOG_WARN("handleRoutingActivated: Unexpected event {} in state {}", fmt::streamed(event), fmt::streamed(m_state));
}

void DoIPServerStateMachine::handleWaitAliveCheckResponse(DoIPEvent event, OptDoIPMessage msg) {
    if (handleCommonTransition(event, msg)) {
        return;
    }

    if (event == DoIPEvent::AliveCheckResponseReceived) {
        DOIP_LOG_INFO("Alive check response received");
        stopTimer(StateMachineTimer::AliveCheck);

        // Restart general inactivity timer
        startGeneralInactivityTimer();

        transitionTo(DoIPState::RoutingActivated);
        return;
    }

    if (event == DoIPEvent::DiagnosticMessageReceived) {
        // Diagnostic message acts as alive check response
        DOIP_LOG_INFO("Diagnostic message received - treating as alive check response");
        stopTimer(StateMachineTimer::AliveCheck);

        if (msg) {
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                sendDiagnosticMessageAck(sourceAddress.value());
            }
        }

        startGeneralInactivityTimer();
        transitionTo(DoIPState::RoutingActivated);
        return;
    }

    DOIP_LOG_WARN("handleWaitAliveCheckResponse: Unexpected event {} in state {}", fmt::streamed(event), fmt::streamed(m_state));
}

void DoIPServerStateMachine::handleFinalize(DoIPEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    DOIP_LOG_INFO("Finalizing state machine...");

    // Connection closure is handled by state entry action
    // Just transition to closed state if needed
    if (m_state != DoIPState::Closed) {
        transitionTo(DoIPState::Closed);
    }
}

bool DoIPServerStateMachine::handleCommonTransition(DoIPEvent event, OptDoIPMessage msg) {
    (void)msg; // Unused parameter

    switch (event) {
    case DoIPEvent::Initial_inactivity_timeout:
        transitionTo(DoIPState::Finalize, DoIPCloseReason::InitialInactivityTimeout);
        break;
    case DoIPEvent::CloseRequestReceived:
        transitionTo(DoIPState::Finalize, DoIPCloseReason::ApplicationRequest);
        break;
    case DoIPEvent::InvalidMessage:
        transitionTo(DoIPState::Finalize, DoIPCloseReason::InvalidMessage);
        break;
    case DoIPEvent::SocketError:
        transitionTo(DoIPState::Finalize, DoIPCloseReason::SocketError);
        break;
    default:
        return false;
    }

    return true;
}

// Helper Methods

void DoIPServerStateMachine::transitionTo(DoIPState new_state, DoIPCloseReason reason) {
    if (m_state == new_state) {
        return;
    }

    assert(reason != DoIPCloseReason::None || new_state != DoIPState::Finalize);

    DoIPState old_state = m_state;
    m_state = new_state;

    DOIP_LOG_INFO("State transition: {} -> {}", fmt::streamed(old_state), fmt::streamed(new_state));

    // Call transition callback if set
    if (m_transitionCallback) {
        m_transitionCallback(old_state, new_state);
    }

    // State entry actions
    switch (new_state) {
    case DoIPState::SocketInitialized:
        transitionTo(DoIPState::WaitRoutingActivation);
        break;

    case DoIPState::WaitRoutingActivation:
        // Start initial inactivity timer
        startTimer(StateMachineTimer::InitialInactivity,
                   m_initialInactivityTimeout);
        break;

    case DoIPState::RoutingActivated:
        // Start general inactivity timer
        startGeneralInactivityTimer();
        break;

    case DoIPState::WaitAliveCheckResponse:
        // Timer already started before transition
        break;

    case DoIPState::Finalize:
        DOIP_LOG_INFO("Entering Finalize state - closing connection");
        stopAllTimers();
        // Close the connection (this will be called only once during transition to Finalize)
        m_context.closeConnection(reason);
        // Immediately transition to Closed
        transitionTo(DoIPState::Closed);
        break;

    case DoIPState::Closed:
        stopAllTimers();
        break;
    }
}

void DoIPServerStateMachine::startTimer(StateMachineTimer timer_id, std::chrono::milliseconds duration) {
    if (m_timerManager.hasTimer(timer_id)) {
        // Stop existing timer if any
        assert(m_timerManager.updateTimer(timer_id, duration));
    }

    // Create timer callback
    auto callback = [this, timer_id]() {
        handleTimeout(timer_id);
    };

    // Add timer to manager
    auto timerId = m_timerManager.addTimer(timer_id, duration, callback, false);

    if (timerId.has_value()) {
        DOIP_LOG_DEBUG("Started timer " + std::to_string(static_cast<int>(timer_id)) + " for " + std::to_string(duration.count()) + "ms");
    } else {
        DOIP_LOG_ERROR("Failed to start timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopTimer(StateMachineTimer timer_id) {
    if (m_timerManager.disableTimer(timer_id)) {
        DOIP_LOG_DEBUG("Stopped timer {}", fmt::streamed(timer_id));
    }
}

void DoIPServerStateMachine::stopAllTimers() {
    // Stop all known timers
    m_timerManager.removeTimer(StateMachineTimer::InitialInactivity);
    m_timerManager.removeTimer(StateMachineTimer::GeneralInactivity);
    m_timerManager.removeTimer(StateMachineTimer::AliveCheck);

    DOIP_LOG_DEBUG("Stopped all timers");
}

ssize_t DoIPServerStateMachine::sendRoutingActivationResponse(const DoIPAddress &source_address, DoIPRoutingActivationResult response_code) {
    // Create routing activation response message
    DoIPAddress serverAddr = m_context.getServerAddress();

    // Build response payload manually
    ByteArray payload;
    source_address.appendTo(payload);
    serverAddr.appendTo(payload);
    payload.push_back(static_cast<uint8_t>(response_code));
    // Reserved 4 bytes
    payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});

    DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    auto sentBytes = sendMessage(response);
    DOIP_LOG_INFO("Sent routing activation response: code=" + std::to_string(static_cast<unsigned int>(response_code)) + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
    return sentBytes;
}

ssize_t DoIPServerStateMachine::sendAliveCheckRequest() {
    auto request = message::makeAliveCheckRequest();
    auto sentBytes = sendMessage(request);
    DOIP_LOG_INFO("Sent alive check request");
    return sentBytes;
}

ssize_t DoIPServerStateMachine::sendDiagnosticMessageResponse(const DoIPAddress &sourceAddress, DoIPDiagnosticAck ack) {
    ssize_t sentBytes;
    if (ack.has_value()) {
        sentBytes = sendDiagnosticMessageNack(sourceAddress, ack.value());
    } else {
        sentBytes = sendDiagnosticMessageAck(sourceAddress);
    }
    m_context.notifyDiagnosticAckSent(ack);
    return sentBytes;
}

ssize_t DoIPServerStateMachine::sendDiagnosticMessageAck(const DoIPAddress &source_address) {
    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr = m_context.getClientAddress();

    auto ack = message::makeDiagnosticPositiveResponse(
        sourceAddr,
        targetAddr,
        ByteArray{} // Empty payload for ACK
    );

    auto sentBytes = sendMessage(ack);

    DOIP_LOG_INFO("Sent diagnostic message ACK to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
    return sentBytes;
}

ssize_t DoIPServerStateMachine::sendDiagnosticMessageNack(const DoIPAddress &source_address, DoIPNegativeDiagnosticAck nack_code) {
    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr = m_context.getClientAddress();

    auto nack = message::makeDiagnosticNegativeResponse(
        sourceAddr,
        targetAddr,
        nack_code,
        ByteArray{} // Empty payload
    );

    auto sentBytes = sendMessage(nack);

    DOIP_LOG_INFO("Sent diagnostic message NACK: code=" + std::to_string(static_cast<unsigned int>(nack_code)) + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
    return sentBytes;
}

ssize_t DoIPServerStateMachine::sendMessage(const DoIPMessage &msg) {
    return m_context.sendProtocolMessage(msg);
}

} // namespace doip

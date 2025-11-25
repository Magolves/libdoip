#include "DoIPServerStateMachine.h"
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

    processEvent(event);

    // Process based on current state
    switch (m_state) {
    case DoIPState::SocketInitialized:
        handleSocketInitialized(event, &message);
        break;
    case DoIPState::WaitRoutingActivation:
        handleWaitRoutingActivation(event, &message);
        break;
    case DoIPState::RoutingActivated:
        handleRoutingActivated(event, &message);
        break;
    case DoIPState::WaitAliveCheckResponse:
        handleWaitAliveCheckResponse(event, &message);
        break;
    case DoIPState::Finalize:
        handleFinalize(event, &message);
        break;
    case DoIPState::Closed:
        // No processing in closed state
        break;
    }
}

void DoIPServerStateMachine::processEvent(DoIPEvent event) {
    DOIP_LOG_INFO("processEvent {} in state {}", fmt::streamed(event), fmt::streamed(m_state));

    switch (event) {
    case DoIPEvent::AliveCheckTimeout:
    case DoIPEvent::Initial_inactivity_timeout:
        transitionTo(DoIPState::Finalize);
        break;
    case DoIPEvent::GeneralInactivityTimeout:
        //transitionTo(DoIPState::WaitAliveCheckResponse);
        break;
    default:
        break;
    }

    // Process events based on current state
    switch (m_state) {
    case DoIPState::SocketInitialized:
        handleSocketInitialized(event, nullptr);
        break;
    case DoIPState::WaitRoutingActivation:
        handleWaitRoutingActivation(event, nullptr);
        break;
    case DoIPState::RoutingActivated:
        handleRoutingActivated(event, nullptr);
        break;
    case DoIPState::WaitAliveCheckResponse:
        handleWaitAliveCheckResponse(event, nullptr);
        break;
    case DoIPState::Finalize:
        handleFinalize(event, nullptr);
        break;
    case DoIPState::Closed:
        // No processing in closed state
        break;
    }
}

void DoIPServerStateMachine::handleTimeout(TimerID timer_id) {
    DoIPEvent event = DoIPEvent::SocketError; // Default value

    DOIP_LOG_WARN("Timeout '{}'", fmt::streamed(timer_id));

    switch (timer_id) {
    case TimerID::InitialInactivity:
        event = DoIPEvent::Initial_inactivity_timeout;
        break;
    case TimerID::GeneralInactivity:
        event = DoIPEvent::GeneralInactivityTimeout;
        break;
    case TimerID::AliveCheck:
        event = DoIPEvent::AliveCheckTimeout;
        break;
    }

    processEvent(event);
}

// State Handlers

void DoIPServerStateMachine::handleSocketInitialized(DoIPEvent event, const DoIPMessage *msg) {
    switch (event) {
    case DoIPEvent::RoutingActivationReceived:
        if (msg) {
            // Extract source address from message
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                m_activeSourceAddress = sourceAddress.value().toUint16();

                // Send routing activation response (success)
                sendRoutingActivationResponse(DoIPAddress(m_activeSourceAddress), 0x10);

                // Start general inactivity timer
                stopTimer(TimerID::InitialInactivity);
                startGeneralInactivityTimer();

                transitionTo(DoIPState::RoutingActivated);
            } else {
                // Invalid message - no source address
                sendRoutingActivationResponse(DoIPAddress(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x00)), 0x00); // Unknown source address
            }
        }
        break;

    case DoIPEvent::Initial_inactivity_timeout:
        DOIP_LOG_WARN("Initial inactivity timeout in SocketInitialized state");
        transitionTo(DoIPState::Finalize);
        break;

    case DoIPEvent::InvalidMessage:
    case DoIPEvent::SocketError:
        transitionTo(DoIPState::Finalize);
        break;

    default:
        // Unexpected event in this state
        DOIP_LOG_WARN("Unexpected event in SocketInitialized state");
        break;
    }
}

void DoIPServerStateMachine::handleWaitRoutingActivation(DoIPEvent event, const DoIPMessage *msg) {
    switch (event) {
    case DoIPEvent::RoutingActivationReceived:
        if (msg) {
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                m_activeSourceAddress = sourceAddress.value().toUint16();

                // Send routing activation response
                sendRoutingActivationResponse(DoIPAddress(m_activeSourceAddress), 0x10);

                // Reset inactivity timer
                stopTimer(TimerID::InitialInactivity);
                startGeneralInactivityTimer();

                transitionTo(DoIPState::RoutingActivated);
            }
        }
        break;

    case DoIPEvent::Initial_inactivity_timeout:
        DOIP_LOG_WARN("Initial inactivity timeout - closing connection");
        transitionTo(DoIPState::Finalize);
        break;

    case DoIPEvent::SocketError:
    case DoIPEvent::InvalidMessage:
        transitionTo(DoIPState::Finalize);
        break;

    default:
        break;
    }
}

void DoIPServerStateMachine::handleRoutingActivated(DoIPEvent event, const DoIPMessage *msg) {
    switch (event) {
    case DoIPEvent::DiagnosticMessageReceived:
        if (msg) {
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                auto ack = m_context.handleDiagnosticMessage(*msg);
                sendDiagnosticMessageResponse(sourceAddress.value(), ack);

                // Reset general inactivity timer
                stopTimer(TimerID::GeneralInactivity);
                startGeneralInactivityTimer();
            }
        }
        break;

    case DoIPEvent::GeneralInactivityTimeout:
        DOIP_LOG_INFO("General inactivity timeout - sending alive check");
        sendAliveCheckRequest();
        m_aliveCheckRetryCount = 0;
        startAliveCheckTimer();
        transitionTo(DoIPState::WaitAliveCheckResponse);
        break;

    case DoIPEvent::CloseRequestReceived:
        transitionTo(DoIPState::Finalize);
        break;

    case DoIPEvent::SocketError:
        transitionTo(DoIPState::Finalize);
        break;

    default:
        break;
    }
}

void DoIPServerStateMachine::handleWaitAliveCheckResponse(DoIPEvent event, const DoIPMessage *msg) {
    switch (event) {
    case DoIPEvent::AliveCheckResponseReceived:
        DOIP_LOG_INFO("Alive check response received");
        stopTimer(TimerID::AliveCheck);

        // Restart general inactivity timer
        startGeneralInactivityTimer();

        transitionTo(DoIPState::RoutingActivated);
        break;

    case DoIPEvent::AliveCheckTimeout:
        m_aliveCheckRetryCount++;

        if (m_aliveCheckRetryCount >= times::server::VehicleAnnouncementNumber) {
            DOIP_LOG_WARN("Alive check max retries reached - closing connection");
            transitionTo(DoIPState::Finalize);
        } else {
            DOIP_LOG_INFO("Alive check timeout - retry " + std::to_string(static_cast<unsigned int>(m_aliveCheckRetryCount)));
            sendAliveCheckRequest();
            startAliveCheckTimer();
        }
        break;

    case DoIPEvent::DiagnosticMessageReceived:
        // Diagnostic message acts as alive check response
        DOIP_LOG_INFO("Diagnostic message received - treating as alive check response");
        stopTimer(TimerID::AliveCheck);

        if (msg) {
            auto sourceAddress = msg->getSourceAddress();
            if (sourceAddress.has_value()) {
                sendDiagnosticMessageAck(sourceAddress.value());
            }
        }

        startGeneralInactivityTimer();
        transitionTo(DoIPState::RoutingActivated);
        break;

    case DoIPEvent::SocketError:
        transitionTo(DoIPState::Finalize);
        break;

    default:
        break;
    }
}

void DoIPServerStateMachine::handleFinalize(DoIPEvent event, const DoIPMessage *msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    DOIP_LOG_INFO("Already in Finalize state - ignoring event");

    // Connection closure is handled by state entry action
    // Just transition to closed state if needed
    if (m_state != DoIPState::Closed) {
        transitionTo(DoIPState::Closed);
    }
}

// Helper Methods

void DoIPServerStateMachine::transitionTo(DoIPState new_state) {
    if (m_state == new_state) {
        return;
    }

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
        // Start initial inactivity timer
        startTimer(TimerID::InitialInactivity,
                   std::chrono::milliseconds(m_initialInactivityTimeout));
        transitionTo(DoIPState::WaitRoutingActivation);
        break;

    case DoIPState::WaitRoutingActivation:
        // Start initial inactivity timer
        startTimer(TimerID::InitialInactivity,
                   std::chrono::milliseconds(m_initialInactivityTimeout));
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
        m_context.closeConnection(CloseReason::GeneralInactivityTimeout);
        // Immediately transition to Closed
        transitionTo(DoIPState::Closed);
        break;

    case DoIPState::Closed:
        stopAllTimers();
        break;
    }
}

void DoIPServerStateMachine::startTimer(TimerID timer_id, std::chrono::milliseconds duration) {
    // Stop existing timer if any
    stopTimer(timer_id);

    // Create timer callback
    auto callback = [this, timer_id]() {
        handleTimeout(timer_id);
    };

    // Add timer to manager
    auto timerId = m_timerManager.addTimer(duration, callback, false);

    if (timerId.has_value()) {
        DOIP_LOG_DEBUG("Started timer " + std::to_string(static_cast<int>(timer_id)) + " for " + std::to_string(duration.count()) + "ms");
    } else {
        DOIP_LOG_ERROR("Failed to start timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopTimer(TimerID timer_id) {
    if (m_timerManager.disableTimer(static_cast<int>(timer_id))) {
        DOIP_LOG_DEBUG("Stopped timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopAllTimers() {
    // Stop all known timers
    m_timerManager.removeTimer(static_cast<int>(TimerID::InitialInactivity));
    m_timerManager.removeTimer(static_cast<int>(TimerID::GeneralInactivity));
    m_timerManager.removeTimer(static_cast<int>(TimerID::AliveCheck));


    DOIP_LOG_DEBUG("Stopped all timers");
}

void DoIPServerStateMachine::sendRoutingActivationResponse(const DoIPAddress &source_address, uint8_t response_code) {
    // Create routing activation response message
    DoIPAddress serverAddr = m_context.getServerAddress();

    // Build response payload manually
    ByteArray payload;
    source_address.appendTo(payload);
    serverAddr.appendTo(payload);
    payload.push_back(response_code);
    // Reserved 4 bytes
    payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});

    DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    m_context.sendProtocolMessage(response);

    DOIP_LOG_INFO("Sent routing activation response: code=" + std::to_string(static_cast<unsigned int>(response_code)) + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

void DoIPServerStateMachine::sendAliveCheckRequest() {
    auto request = message::makeAliveCheckRequest();
    m_context.sendProtocolMessage(request);

    DOIP_LOG_INFO("Sent alive check request");
}

void DoIPServerStateMachine::sendDiagnosticMessageResponse(const DoIPAddress& sourceAddress, DoIPDiagnosticAck ack) {
    if (ack.has_value()) {
        sendDiagnosticMessageNack(sourceAddress, ack.value());
    } else {
        sendDiagnosticMessageAck(sourceAddress);
    }
}

void DoIPServerStateMachine::sendDiagnosticMessageAck(const DoIPAddress &source_address) {
    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr(m_activeSourceAddress);

    auto ack = message::makeDiagnosticPositiveResponse(
        sourceAddr,
        targetAddr,
        ByteArray{} // Empty payload for ACK
    );

    notifyMessage(ack);

    DOIP_LOG_INFO("Sent diagnostic message ACK to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

void DoIPServerStateMachine::sendDiagnosticMessageNack(const DoIPAddress &source_address, DoIPNegativeDiagnosticAck nack_code) {
    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr(m_activeSourceAddress);

    auto nack = message::makeDiagnosticNegativeResponse(
        sourceAddr,
        targetAddr,
        static_cast<DoIPNegativeDiagnosticAck>(nack_code),
        ByteArray{} // Empty payload
    );

    notifyMessage(nack);

    DOIP_LOG_INFO("Sent diagnostic message NACK: code=" + std::to_string(static_cast<unsigned int>(nack_code)) + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

void DoIPServerStateMachine::notifyMessage(const DoIPMessage &msg) {
    m_context.sendProtocolMessage(msg);
}

} // namespace doip

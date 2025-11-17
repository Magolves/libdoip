#include "DoIPServerStateMachine.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"

namespace doip {

DoIPServerStateMachine::~DoIPServerStateMachine() {
    stopAllTimers();
}

void DoIPServerStateMachine::processMessage(const DoIPMessage& message) {
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
            event = DoIPEvent::Invalid_message;
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
    DoIPEvent event = DoIPEvent::Socket_error; // Default value

    switch (timer_id) {
        case TimerID::InitialInactivity:
            event = DoIPEvent::Initial_inactivity_timeout;
            break;
        case TimerID::GeneralInactivity:
            event = DoIPEvent::General_inactivity_timeout;
            break;
        case TimerID::AliveCheck:
            event = DoIPEvent::Alive_check_timeout;
            break;
    }

    processEvent(event);
}

// State Handlers

void DoIPServerStateMachine::handleSocketInitialized(DoIPEvent event, const DoIPMessage* msg) {
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
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

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

        case DoIPEvent::Invalid_message:
        case DoIPEvent::Socket_error:
            transitionTo(DoIPState::Finalize);
            break;

        default:
            // Unexpected event in this state
            DOIP_LOG_WARN("Unexpected event in SocketInitialized state");
            break;
    }
}

void DoIPServerStateMachine::handleWaitRoutingActivation(DoIPEvent event, const DoIPMessage* msg) {
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
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

                    transitionTo(DoIPState::RoutingActivated);
                }
            }
            break;

        case DoIPEvent::Initial_inactivity_timeout:
            DOIP_LOG_WARN("Initial inactivity timeout - closing connection");
            transitionTo(DoIPState::Finalize);
            break;

        case DoIPEvent::Socket_error:
        case DoIPEvent::Invalid_message:
            transitionTo(DoIPState::Finalize);
            break;

        default:
            break;
    }
}

void DoIPServerStateMachine::handleRoutingActivated(DoIPEvent event, const DoIPMessage* msg) {
    switch (event) {
        case DoIPEvent::DiagnosticMessageReceived:
            if (msg) {
                auto sourceAddress = msg->getSourceAddress();
                if (sourceAddress.has_value()) {
                    // Send diagnostic message acknowledgment
                    sendDiagnosticMessageAck(sourceAddress.value());

                    // Reset general inactivity timer
                    stopTimer(TimerID::GeneralInactivity);
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
                }
            }
            break;

        case DoIPEvent::General_inactivity_timeout:
            DOIP_LOG_INFO("General inactivity timeout - sending alive check");
            sendAliveCheckRequest();
            m_aliveCheckRetryCount = 0;
            startTimer(TimerID::AliveCheck,
                      std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
            transitionTo(DoIPState::WaitAliveCheckResponse);
            break;

        case DoIPEvent::CloseRequestReceived:
            transitionTo(DoIPState::Finalize);
            break;

        case DoIPEvent::Socket_error:
            transitionTo(DoIPState::Finalize);
            break;

        default:
            break;
    }
}

void DoIPServerStateMachine::handleWaitAliveCheckResponse(DoIPEvent event, const DoIPMessage* msg) {
    switch (event) {
        case DoIPEvent::AliveCheckResponseReceived:
            DOIP_LOG_INFO("Alive check response received");
            stopTimer(TimerID::AliveCheck);

            // Restart general inactivity timer
            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

            transitionTo(DoIPState::RoutingActivated);
            break;

        case DoIPEvent::Alive_check_timeout:
            m_aliveCheckRetryCount++;

            if (m_aliveCheckRetryCount >= times::server::VehicleAnnouncementNumber) {
                DOIP_LOG_WARN("Alive check max retries reached - closing connection");
                transitionTo(DoIPState::Finalize);
            } else {
                DOIP_LOG_INFO("Alive check timeout - retry "
                        + std::to_string(static_cast<unsigned int>(m_aliveCheckRetryCount)));
                sendAliveCheckRequest();
                startTimer(TimerID::AliveCheck,
                          std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
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

            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
            transitionTo(DoIPState::RoutingActivated);
            break;

        case DoIPEvent::Socket_error:
            transitionTo(DoIPState::Finalize);
            break;

        default:
            break;
    }
}

void DoIPServerStateMachine::handleFinalize(DoIPEvent event, const DoIPMessage* msg) {
    (void)event;  // Unused parameter
    (void)msg;    // Unused parameter

    // Stop all timers
    stopAllTimers();

    // Transition to closed state
    transitionTo(DoIPState::Closed);
}

// Helper Methods

void DoIPServerStateMachine::transitionTo(DoIPState new_state) {
    if (m_state == new_state) {
        return;
    }

    DoIPState old_state = m_state;
    m_state = new_state;

    DOIP_LOG_INFO("State transition: " + std::to_string(static_cast<int>(old_state))
            + " -> " + std::to_string(static_cast<int>(new_state)));

    // Call transition callback if set
    if (m_transitionCallback) {
        m_transitionCallback(old_state, new_state);
    }

    // State entry actions
    switch (new_state) {
        case DoIPState::SocketInitialized:
            // Start initial inactivity timer
            startTimer(TimerID::InitialInactivity,
                      std::chrono::milliseconds(times::server::InitialInactivityTimeout));
            break;

        case DoIPState::WaitRoutingActivation:
            // Start initial inactivity timer
            startTimer(TimerID::InitialInactivity,
                      std::chrono::milliseconds(times::server::InitialInactivityTimeout));
            break;

        case DoIPState::RoutingActivated:
            // Start general inactivity timer
            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
            break;

        case DoIPState::WaitAliveCheckResponse:
            // Timer already started before transition
            break;

        case DoIPState::Finalize:
            stopAllTimers();
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
        DOIP_LOG_DEBUG("Started timer " + std::to_string(static_cast<int>(timer_id))
                 + " for " + std::to_string(duration.count()) + "ms");
    } else {
        DOIP_LOG_ERROR("Failed to start timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopTimer(TimerID timer_id) {
    m_timerManager.disableTimer(static_cast<int>(timer_id));
    DOIP_LOG_DEBUG("Stopped timer " + std::to_string(static_cast<int>(timer_id)));
}

void DoIPServerStateMachine::stopAllTimers() {
    // Stop all known timers
    m_timerManager.disableTimer(static_cast<int>(TimerID::InitialInactivity));
    m_timerManager.disableTimer(static_cast<int>(TimerID::GeneralInactivity));
    m_timerManager.disableTimer(static_cast<int>(TimerID::AliveCheck));
    DOIP_LOG_DEBUG("Stopped all timers");
}

void DoIPServerStateMachine::sendRoutingActivationResponse(DoIPAddress source_address, uint8_t response_code) {
    if (!m_sendMessageCallback) {
        DOIP_LOG_ERROR("Send message callback not set");
        return;
    }

    // Create routing activation response message
    DoIPAddress serverAddr(0x0E, 0x80);  // Default server address (0x0E80)

    // Build response payload manually
    ByteArray payload;
    source_address.appendTo(payload);
    serverAddr.appendTo(payload);
    payload.push_back(response_code);
    // Reserved 4 bytes
    payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});

    DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    m_sendMessageCallback(response);

    DOIP_LOG_INFO("Sent routing activation response: code="
            + std::to_string(static_cast<unsigned int>(response_code))
            + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

void DoIPServerStateMachine::sendAliveCheckRequest() {
    if (!m_sendMessageCallback) {
        DOIP_LOG_ERROR("Send message callback not set");
        return;
    }

    auto request = message::makeAliveCheckRequest();
    m_sendMessageCallback(request);

    DOIP_LOG_INFO("Sent alive check request");
}

void DoIPServerStateMachine::sendDiagnosticMessageAck(DoIPAddress source_address) {
    if (!m_sendMessageCallback) {
        DOIP_LOG_ERROR("Send message callback not set");
        return;
    }

    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr(m_activeSourceAddress);

    auto ack = message::makeDiagnosticPositiveResponse(
        sourceAddr,
        targetAddr,
        ByteArray{}  // Empty payload for ACK
    );

    m_sendMessageCallback(ack);

    DOIP_LOG_INFO("Sent diagnostic message ACK to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

void DoIPServerStateMachine::sendDiagnosticMessageNack(DoIPAddress source_address, uint8_t nack_code) {
    if (!m_sendMessageCallback) {
        DOIP_LOG_ERROR("Send message callback not set");
        return;
    }

    DoIPAddress sourceAddr = source_address;
    DoIPAddress targetAddr(m_activeSourceAddress);

    auto nack = message::makeDiagnosticNegativeResponse(
        sourceAddr,
        targetAddr,
        static_cast<DoIPNegativeDiagnosticAck>(nack_code),
        ByteArray{}  // Empty payload
    );

    m_sendMessageCallback(nack);

    DOIP_LOG_INFO("Sent diagnostic message NACK: code="
            + std::to_string(static_cast<unsigned int>(nack_code))
            + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
}

} // namespace doip

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
        case DoIPState::Socket_initialized:
            handleSocketInitialized(event, &message);
            break;
        case DoIPState::Wait_routing_activation:
            handleWaitRoutingActivation(event, &message);
            break;
        case DoIPState::Routing_activated:
            handleRoutingActivated(event, &message);
            break;
        case DoIPState::Wait_alive_check_response:
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
        case DoIPState::Socket_initialized:
            handleSocketInitialized(event, nullptr);
            break;
        case DoIPState::Wait_routing_activation:
            handleWaitRoutingActivation(event, nullptr);
            break;
        case DoIPState::Routing_activated:
            handleRoutingActivated(event, nullptr);
            break;
        case DoIPState::Wait_alive_check_response:
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
    DoIPEvent event;

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
                    active_source_address_ = sourceAddress.value().toUint16();

                    // Send routing activation response (success)
                    sendRoutingActivationResponse(active_source_address_, 0x10);

                    // Start general inactivity timer
                    stopTimer(TimerID::InitialInactivity);
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

                    transitionTo(DoIPState::Routing_activated);
                } else {
                    // Invalid message - no source address
                    sendRoutingActivationResponse(0x0000, 0x00); // Unknown source address
                }
            }
            break;

        case DoIPEvent::Initial_inactivity_timeout:
            LOG_WARNING("Initial inactivity timeout in Socket_initialized state");
            transitionTo(DoIPState::Finalize);
            break;

        case DoIPEvent::Invalid_message:
        case DoIPEvent::Socket_error:
            transitionTo(DoIPState::Finalize);
            break;

        default:
            // Unexpected event in this state
            LOG_WARNING("Unexpected event in Socket_initialized state");
            break;
    }
}

void DoIPServerStateMachine::handleWaitRoutingActivation(DoIPEvent event, const DoIPMessage* msg) {
    switch (event) {
        case DoIPEvent::RoutingActivationReceived:
            if (msg) {
                auto sourceAddress = msg->getSourceAddress();
                if (sourceAddress.has_value()) {
                    active_source_address_ = sourceAddress.value().toUint16();

                    // Send routing activation response
                    sendRoutingActivationResponse(active_source_address_, 0x10);

                    // Reset inactivity timer
                    stopTimer(TimerID::InitialInactivity);
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

                    transitionTo(DoIPState::Routing_activated);
                }
            }
            break;

        case DoIPEvent::Initial_inactivity_timeout:
            LOG_WARNING("Initial inactivity timeout - closing connection");
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
                    sendDiagnosticMessageAck(sourceAddress.value().toUint16());

                    // Reset general inactivity timer
                    stopTimer(TimerID::GeneralInactivity);
                    startTimer(TimerID::GeneralInactivity,
                              std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
                }
            }
            break;

        case DoIPEvent::General_inactivity_timeout:
            LOG_INFO("General inactivity timeout - sending alive check");
            sendAliveCheckRequest();
            alive_check_retry_count_ = 0;
            startTimer(TimerID::AliveCheck,
                      std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
            transitionTo(DoIPState::Wait_alive_check_response);
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
            LOG_INFO("Alive check response received");
            stopTimer(TimerID::AliveCheck);

            // Restart general inactivity timer
            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));

            transitionTo(DoIPState::Routing_activated);
            break;

        case DoIPEvent::Alive_check_timeout:
            alive_check_retry_count_++;

            if (alive_check_retry_count_ >= times::server::VehicleAnnouncementNumber) {
                LOG_WARNING("Alive check max retries reached - closing connection");
                transitionTo(DoIPState::Finalize);
            } else {
                LOG_INFO("Alive check timeout - retry "
                        + std::to_string(alive_check_retry_count_));
                sendAliveCheckRequest();
                startTimer(TimerID::AliveCheck,
                          std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
            }
            break;

        case DoIPEvent::DiagnosticMessageReceived:
            // Diagnostic message acts as alive check response
            LOG_INFO("Diagnostic message received - treating as alive check response");
            stopTimer(TimerID::AliveCheck);

            if (msg) {
                auto sourceAddress = msg->getSourceAddress();
                if (sourceAddress.has_value()) {
                    sendDiagnosticMessageAck(sourceAddress.value().toUint16());
                }
            }

            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
            transitionTo(DoIPState::Routing_activated);
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

    LOG_INFO("State transition: " + std::to_string(static_cast<int>(old_state))
            + " -> " + std::to_string(static_cast<int>(new_state)));

    // Call transition callback if set
    if (m_transition_callback) {
        m_transition_callback(old_state, new_state);
    }

    // State entry actions
    switch (new_state) {
        case DoIPState::Socket_initialized:
            // Start initial inactivity timer
            startTimer(TimerID::InitialInactivity,
                      std::chrono::milliseconds(times::server::InitialInactivityTimeout));
            break;

        case DoIPState::Wait_routing_activation:
            // Start initial inactivity timer
            startTimer(TimerID::InitialInactivity,
                      std::chrono::milliseconds(times::server::InitialInactivityTimeout));
            break;

        case DoIPState::Routing_activated:
            // Start general inactivity timer
            startTimer(TimerID::GeneralInactivity,
                      std::chrono::milliseconds(times::server::GeneralInactivityTimeout));
            break;

        case DoIPState::Wait_alive_check_response:
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
        LOG_DEBUG("Started timer " + std::to_string(static_cast<int>(timer_id))
                 + " for " + std::to_string(duration.count()) + "ms");
    } else {
        LOG_ERROR("Failed to start timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopTimer(TimerID timer_id) {
    auto it = timers_.find(timer_id);
    if (it != timers_.end()) {
        // Timer exists - remove it
        timers_.erase(it);
        LOG_DEBUG("Stopped timer " + std::to_string(static_cast<int>(timer_id)));
    }
}

void DoIPServerStateMachine::stopAllTimers() {
    timers_.clear();
    LOG_DEBUG("Stopped all timers");
}

void DoIPServerStateMachine::sendRoutingActivationResponse(uint16_t source_address, uint8_t response_code) {
    if (!send_message_callback_) {
        LOG_ERROR("Send message callback not set");
        return;
    }

    // Create routing activation response message
    DoIPAddress sourceAddr(source_address);
    DoIPAddress serverAddr(0x0E80);  // Default server address

    auto response = message::makeRoutingActivationResponse(
        sourceAddr,
        serverAddr,
        response_code,
        0x00  // Reserved byte
    );

    send_message_callback_(response);

    LOG_INFO("Sent routing activation response: code="
            + std::to_string(response_code)
            + " to address=" + std::to_string(source_address));
}

void DoIPServerStateMachine::sendAliveCheckRequest() {
    if (!send_message_callback_) {
        LOG_ERROR("Send message callback not set");
        return;
    }

    auto request = message::makeAliveCheckRequest();
    send_message_callback_(request);

    LOG_INFO("Sent alive check request");
}

void DoIPServerStateMachine::sendDiagnosticMessageAck(uint16_t source_address) {
    if (!send_message_callback_) {
        LOG_ERROR("Send message callback not set");
        return;
    }

    DoIPAddress sourceAddr(source_address);
    DoIPAddress targetAddr(active_source_address_);

    auto ack = message::makeDiagnosticPositiveResponse(
        sourceAddr,
        targetAddr,
        ByteArray{}  // Empty payload for ACK
    );

    send_message_callback_(ack);

    LOG_INFO("Sent diagnostic message ACK to address=" + std::to_string(source_address));
}

void DoIPServerStateMachine::sendDiagnosticMessageNack(uint16_t source_address, uint8_t nack_code) {
    if (!send_message_callback_) {
        LOG_ERROR("Send message callback not set");
        return;
    }

    DoIPAddress sourceAddr(source_address);
    DoIPAddress targetAddr(active_source_address_);

    auto nack = message::makeDiagnosticNegativeResponse(
        sourceAddr,
        targetAddr,
        static_cast<DoIPNegativeDiagnosticAck>(nack_code),
        ByteArray{}  // Empty payload
    );

    send_message_callback_(nack);

    LOG_INFO("Sent diagnostic message NACK: code="
            + std::to_string(nack_code)
            + " to address=" + std::to_string(source_address));
}

} // namespace doip

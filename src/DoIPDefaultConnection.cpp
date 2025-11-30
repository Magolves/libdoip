#include "DoIPDefaultConnection.h"
#include "Logger.h"

#include <execinfo.h>
#include <iostream>

namespace doip {

DoIPDefaultConnection::DoIPDefaultConnection(UniqueServerModelPtr model)
    : m_serverModel(std::move(model)),
      STATE_DESCRIPTORS{
          StateDescriptor(
              DoIPServerState::SocketInitialized,
              ConnectionTimers::UserDefined,
              DoIPServerState::WaitRoutingActivation,
              [this](std::optional<DoIPMessage> msg) { this->handleSocketInitialized(DoIPServerEvent{}, msg); },
              nullptr),
          StateDescriptor(
              DoIPServerState::WaitRoutingActivation,
              ConnectionTimers::InitialInactivity,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitRoutingActivation(DoIPServerEvent{}, msg); },
              nullptr),
          StateDescriptor(
              DoIPServerState::RoutingActivated,
              ConnectionTimers::GeneralInactivity,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleRoutingActivated(DoIPServerEvent{}, msg); },
              nullptr),
          StateDescriptor(
              DoIPServerState::WaitAliveCheckResponse,
              ConnectionTimers::AliveCheck,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitAliveCheckResponse(DoIPServerEvent{}, msg); },
              nullptr),
          StateDescriptor(
              DoIPServerState::WaitDownstreamResponse,
              ConnectionTimers::UserDefined,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitDownstreamResponse(DoIPServerEvent{}, msg); },
              nullptr,
              2s),
          StateDescriptor(
              DoIPServerState::Finalize,
              ConnectionTimers::UserDefined,
              DoIPServerState::Closed,
              [this](std::optional<DoIPMessage> msg) { this->handleFinalize(DoIPServerEvent{}, msg); },
              nullptr),
          StateDescriptor(
              DoIPServerState::Closed,
              ConnectionTimers::UserDefined,
              DoIPServerState::Closed,
              nullptr,
              nullptr)} {
    m_isOpen = true;
    m_serverModel->onOpenConnection(*this);
    m_state = &STATE_DESCRIPTORS[0];

    DOIP_LOG_INFO("Default connection created, transitioning to SocketInitialized state...");
    transitionTo(DoIPServerState::WaitRoutingActivation);
}

ssize_t DoIPDefaultConnection::sendProtocolMessage(const DoIPMessage &msg) {
    DOIP_LOG_INFO("Default connection: Sending protocol message: {}", fmt::streamed(msg));
    return static_cast<ssize_t>(msg.size()); // Simulate sending by returning the message size
}

void DoIPDefaultConnection::closeConnection(DoIPCloseReason reason) {
    try {
        DOIP_LOG_INFO("Default connection: Closing connection, reason: {}", fmt::streamed(reason));
        transitionTo(DoIPServerState::Closed);
        m_closeReason = reason;
        m_timerManager.stopAll();
        notifyConnectionClosed(reason);
    } catch (const std::exception &e) {
        DOIP_LOG_ERROR("Error notifying connection closed: {}", e.what());

        void *callstack[128];
        int frames = backtrace(callstack, 128);
        char **strs = backtrace_symbols(callstack, frames);

        DOIP_LOG_ERROR("Exception during closeConnection: {}", e.what());
        DOIP_LOG_ERROR("Stack trace:");
        for (int i = 0; i < frames; ++i) {
            DOIP_LOG_ERROR("{}", strs[i]);
        }
        free(strs);
    }
    m_isOpen = false;
}

DoIPAddress DoIPDefaultConnection::getServerAddress() const {
    return m_serverModel->serverAddress;
}

DoIPAddress DoIPDefaultConnection::getClientAddress() const {
    return m_routedClientAddress;
}

void DoIPDefaultConnection::setClientAddress(const DoIPAddress &address) {
    m_routedClientAddress = address;
}

void DoIPDefaultConnection::handleMessage2(const DoIPMessage &message) {
    m_state->messageHandler(message);
}

void DoIPDefaultConnection::transitionTo(DoIPServerState newState) {
    if (m_state->state == newState) {
        return;
    }

    auto it = std::find_if(
        STATE_DESCRIPTORS.begin(),
        STATE_DESCRIPTORS.end(),
        [newState](const StateDescriptor &desc) {
            return desc.state == newState;
        });
    if (it != STATE_DESCRIPTORS.end()) {
        DOIP_LOG_INFO("Transitioning from state {} to state {}", fmt::streamed(m_state->state), fmt::streamed(newState));
        m_state = &(*it);
        startStateTimer(m_state);
    } else {
        DOIP_LOG_ERROR("Invalid state transition to {}", fmt::streamed(newState));
    }
}

void DoIPDefaultConnection::startStateTimer(StateDescriptor const *stateDesc) {
    assert(stateDesc != nullptr);
    if (stateDesc == nullptr) {
        return;
    }

    m_timerManager.stopAll();

    // todo: Optimize timer table lookup (and content)
    std::chrono::milliseconds duration = std::chrono::milliseconds(0);
    auto timerDesc = std::find_if(
        m_timerDurations.begin(),
        m_timerDurations.end(),
        [stateDesc](const auto &pair) { return pair.first == stateDesc->timer; });

    if (timerDesc != m_timerDurations.end()) {
        duration = timerDesc->second;
    }

    if (stateDesc->timer == ConnectionTimers::UserDefined) {
        duration = stateDesc->timeoutDurationUser;
    }

    if (duration.count() == 0) {
        DOIP_LOG_DEBUG("User-defined timer duration is zero, transitioning immediately to state {}", fmt::streamed(stateDesc->stateAfterTimeout));
        transitionTo(stateDesc->stateAfterTimeout);
        return;
    }

    DOIP_LOG_DEBUG("Starting timer for state {}: Timer ID {}, duration {}ms", fmt::streamed(stateDesc->state), fmt::streamed(stateDesc->timer), duration.count());

    std::function<void(ConnectionTimers)> timeoutHandler = [this](ConnectionTimers timerId) { handleTimeout(timerId); };

    if (stateDesc->timeoutHandler != nullptr) {
        timeoutHandler = stateDesc->timeoutHandler;
        DOIP_LOG_ERROR("Use non-default timer {}", fmt::streamed(timerDesc->first));
    }

    auto id = m_timerManager.addTimer(
        timerDesc->first, duration, timeoutHandler, false);

    if (id.has_value()) {
        DOIP_LOG_DEBUG("Started timer {} for {}ms", fmt::streamed(timerDesc->first), duration.count());
    } else {
        DOIP_LOG_ERROR("Failed to start timer {}", fmt::streamed(timerDesc->first));
    }
}

void DoIPDefaultConnection::restartStateTimer() {
    assert(m_state != nullptr);
    if (!m_timerManager.restartTimer(m_state->timer)) {
        DOIP_LOG_ERROR("Failed to restart timer {}", fmt::streamed(m_state->timer));
    }
}

void DoIPDefaultConnection::handleSocketInitialized(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    transitionTo(DoIPServerState::WaitRoutingActivation);
}

void DoIPDefaultConnection::handleWaitRoutingActivation(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling routing activation would go here
    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto sourceAddress = msg->getSourceAddress();
    bool hasAddress = sourceAddress.has_value();
    bool rightPayloadType = (msg->getPayloadType() == DoIPPayloadType::RoutingActivationRequest);

    if (!hasAddress || !rightPayloadType) {
        DOIP_LOG_WARN("Invalid Routing Activation Request message");
        closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }

    // Set client address in context
    setClientAddress(sourceAddress.value());
    // Send Routing Activation Response
    sendRoutingActivationResponse(sourceAddress.value(), DoIPRoutingActivationResult::RouteActivated);
    // Transition to Routing Activated state
    transitionTo(DoIPServerState::RoutingActivated);
}

void DoIPDefaultConnection::handleRoutingActivated(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter

    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto message = msg.value();

    switch (message.getPayloadType()) {
    case DoIPPayloadType::DiagnosticMessage:
        break;
    case DoIPPayloadType::AliveCheckResponse:
        restartStateTimer();
        return;
    default:
        DOIP_LOG_WARN("Received unsupported message type {} in Routing Activated state", fmt::streamed(message.getPayloadType()));
        sendDiagnosticMessageResponse(DoIPAddress::ZeroAddress, DoIPNegativeDiagnosticAck::TransportProtocolError);
        //closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }
    auto sourceAddress = message.getSourceAddress();
    if (!sourceAddress.has_value()) {
        closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }
    if (sourceAddress.value() != getClientAddress()) {
        DOIP_LOG_WARN("Received diagnostic message from unexpected source address {}", fmt::streamed(sourceAddress.value()));
        sendDiagnosticMessageResponse(sourceAddress.value(), DoIPNegativeDiagnosticAck::InvalidSourceAddress);
        //closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }

    sendDiagnosticMessageResponse(sourceAddress.value(), std::nullopt);
    auto ack = notifyDiagnosticMessage(*msg);

    // Reset general inactivity timer
    restartStateTimer();
    // if (m_context.hasDownstreamHandler()) {
    //     startDownstreamResponseTimer();
    //     auto result = m_context.notifyDownstreamRequest(*msg);
    //     if (result == DoIPDownstreamResult::Pending) {
    //         transitionTo(DoIPServerState::WaitDownstreamResponse);
    //     } else if (result == DoIPDownstreamResult::Error) {
    //         sendDiagnosticMessageResponse(sourceAddress.value(), DoIPNegativeDiagnosticAck::TargetUnreachable);
    //     }
    // }
}

void DoIPDefaultConnection::handleWaitAliveCheckResponse(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling wait alive check response would go here
    (void)event; // Unused parameter

    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto message = msg.value();

    switch (message.getPayloadType()) {
    case DoIPPayloadType::DiagnosticMessage: /* fall-through expected */
    case DoIPPayloadType::AliveCheckResponse:
        transitionTo(DoIPServerState::RoutingActivated);
        return;
    default:
        DOIP_LOG_WARN("Received unsupported message type {} in Wait Alive Check Response state", fmt::streamed(message.getPayloadType()));
        sendDiagnosticMessageResponse(DoIPAddress::ZeroAddress, DoIPNegativeDiagnosticAck::TransportProtocolError);
        //closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }
}

void DoIPDefaultConnection::handleWaitDownstreamResponse(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling wait downstream response would go here
}

void DoIPDefaultConnection::handleFinalize(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling finalize would go here
    transitionTo(DoIPServerState::Closed);
}

void DoIPDefaultConnection::handleTimeout(ConnectionTimers timer_id) {
    DOIP_LOG_WARN("SM2 Timeout '{}'", fmt::streamed(timer_id));

    switch (timer_id) {
    case ConnectionTimers::InitialInactivity:
        closeConnection(DoIPCloseReason::InitialInactivityTimeout);
        break;
    case ConnectionTimers::GeneralInactivity:
        closeConnection(DoIPCloseReason::GeneralInactivityTimeout);
        break;
    case ConnectionTimers::AliveCheck:
        closeConnection(DoIPCloseReason::AliveCheckTimeout);
        break;
    case ConnectionTimers::DownstreamResponse:
        DOIP_LOG_WARN("Downstream response timeout occurred");
        transitionTo(DoIPServerState::RoutingActivated);

        break;
    case ConnectionTimers::UserDefined:
        DOIP_LOG_WARN("User-defined timer -> must be handled separately");
        break;
    default:
        DOIP_LOG_ERROR("Unhandled timeout for timer id {}", fmt::streamed(timer_id));
        break;
    }
}

ssize_t DoIPDefaultConnection::sendRoutingActivationResponse(const DoIPAddress &source_address, DoIPRoutingActivationResult response_code) {
    // Create routing activation response message
    DoIPAddress serverAddr = getServerAddress();

    // Build response payload manually
    ByteArray payload;
    source_address.appendTo(payload);
    serverAddr.appendTo(payload);
    payload.push_back(static_cast<uint8_t>(response_code));
    // Reserved 4 bytes
    payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});

    DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    auto sentBytes = sendProtocolMessage(response);
    DOIP_LOG_INFO("Sent routing activation response: code=" + std::to_string(static_cast<unsigned int>(response_code)) + " to address=" + std::to_string(static_cast<unsigned int>(source_address.toUint16())));
    return -1;
}

ssize_t DoIPDefaultConnection::sendAliveCheckRequest() {
    auto request = message::makeAliveCheckRequest();
    auto sentBytes = sendProtocolMessage(request);
    DOIP_LOG_INFO("Sent alive check request");
    return sentBytes;
}

ssize_t DoIPDefaultConnection::sendDiagnosticMessageResponse(const DoIPAddress &sourceAddress, DoIPDiagnosticAck ack) {
    ssize_t sentBytes;
    DoIPAddress targetAddress = getServerAddress();
    DoIPMessage message;

    if (ack.has_value()) {
        message = message::makeDiagnosticNegativeResponse(
            sourceAddress,
            targetAddress,
            ack.value(),
            ByteArray{} // Empty payload
        );
    } else {
        message = message::makeDiagnosticPositiveResponse(
            sourceAddress,
            targetAddress,
            ByteArray{} // Empty payload for ACK
        );
    }
    sentBytes = sendProtocolMessage(message);
    notifyDiagnosticAckSent(ack);
    return sentBytes;
}

DoIPDiagnosticAck DoIPDefaultConnection::notifyDiagnosticMessage(const DoIPMessage &msg) {
    if (m_serverModel->onDiagnosticMessage) {
        return m_serverModel->onDiagnosticMessage(*this, msg);
    }
    return std::nullopt;
}

void DoIPDefaultConnection::notifyConnectionClosed(DoIPCloseReason reason) {
    if (m_serverModel && m_serverModel->onCloseConnection) {
        m_serverModel->onCloseConnection(*this, reason);
    }
}

void DoIPDefaultConnection::notifyDiagnosticAckSent(DoIPDiagnosticAck ack) {
    if (m_serverModel->onDiagnosticNotification) {
        m_serverModel->onDiagnosticNotification(*this, ack);
    }
}

bool DoIPDefaultConnection::hasDownstreamHandler() const {
    return m_serverModel->hasDownstreamHandler();
}

DoIPDownstreamResult DoIPDefaultConnection::notifyDownstreamRequest(const DoIPMessage &msg) {
    if (m_serverModel->onDownstreamRequest) {
        return m_serverModel->onDownstreamRequest(*this, msg);
    }
    return DoIPDownstreamResult::Error;
}

void DoIPDefaultConnection::receiveDownstreamResponse(const DoIPMessage &response) {
    (void)response;
    // TODO: Implement state machine event processing for downstream response
}

void DoIPDefaultConnection::notifyDownstreamResponseReceived(const DoIPMessage &request, const DoIPMessage &response) {
    if (m_serverModel->onDownstreamResponse) {
        m_serverModel->onDownstreamResponse(*this, request, response);
    }
}

} // namespace doip
#include "DoIPDefaultConnection.h"
#include "Logger.h"

namespace doip {

DoIPDefaultConnection::DoIPDefaultConnection(UniqueServerModelPtr model)
    : m_stateMachine(*this),
      m_serverModel(std::move(model)),
      STATE_DESCRIPTORS{
        StateDescriptor(
            DoIPServerState::SocketInitialized,
            ConnectionTimers::UserDefined,
            DoIPServerState::WaitRoutingActivation,
            [this](std::optional<DoIPMessage> msg) { this->handleSocketInitialized(DoIPServerEvent{}, msg); },
            nullptr
        ),
        StateDescriptor(
            DoIPServerState::WaitRoutingActivation,
            ConnectionTimers::InitialInactivity,
            DoIPServerState::Finalize,
            [this](std::optional<DoIPMessage> msg) { this->handleWaitRoutingActivation(DoIPServerEvent{}, msg); },
            nullptr
        ),
        StateDescriptor(
            DoIPServerState::RoutingActivated,
            ConnectionTimers::GeneralInactivity,
            DoIPServerState::Finalize,
            [this](std::optional<DoIPMessage> msg) { this->handleRoutingActivated(DoIPServerEvent{}, msg); },
            nullptr
        ),
        StateDescriptor(
            DoIPServerState::WaitAliveCheckResponse,
            ConnectionTimers::AliveCheck,
            DoIPServerState::Finalize,
            [this](std::optional<DoIPMessage> msg) { this->handleWaitAliveCheckResponse(DoIPServerEvent{}, msg); },
            nullptr
        ),
        StateDescriptor(
            DoIPServerState::WaitDownstreamResponse,
            ConnectionTimers::UserDefined,
            DoIPServerState::Finalize,
            [this](std::optional<DoIPMessage> msg) { this->handleWaitDownstreamResponse(DoIPServerEvent{}, msg); },
            nullptr,
            2s
        ),
        StateDescriptor(
            DoIPServerState::Finalize,
            ConnectionTimers::UserDefined,
            DoIPServerState::Closed,
            [this](std::optional<DoIPMessage> msg) { this->handleFinalize(DoIPServerEvent{}, msg); },
            nullptr
        ),
        StateDescriptor(
            DoIPServerState::Closed,
            ConnectionTimers::UserDefined,
            DoIPServerState::Closed,
            nullptr,
            nullptr
        )
      }
{
    m_isOpen = true;
    m_serverModel->onOpenConnection(*this);
    m_state = &STATE_DESCRIPTORS[0];
}

ssize_t DoIPDefaultConnection::sendProtocolMessage(const DoIPMessage &msg) {
    DOIP_LOG_INFO("Default connection: Sending protocol message: {}", fmt::streamed(msg));
    return static_cast<ssize_t>(msg.size()); // Simulate sending by returning the message size
}

void DoIPDefaultConnection::closeConnection(DoIPCloseReason reason) {
    DOIP_LOG_INFO("Default connection: Closing connection, reason: {}", fmt::streamed(reason));
    m_closeReason = reason;
    m_isOpen = false;
    notifyConnectionClosed(reason);
}

DoIPAddress DoIPDefaultConnection::getServerAddress() const {
    return m_serverModel->serverAddress;
}

DoIPAddress DoIPDefaultConnection::getClientAddress() const {
    return m_routedClientAddress;
}

void DoIPDefaultConnection::setClientAddress(const DoIPAddress& address) {
    m_routedClientAddress = address;
}

void DoIPDefaultConnection::handleMessage(const DoIPMessage &message) {
    m_state->messageHandler(message);
}

void DoIPDefaultConnection::transitionTo(DoIPServerState newState, DoIPCloseReason reason) {
    //DOIP_LOG_INFO("Transitioning from state {} to state {}", fmt::streamed(m_state.state), fmt::streamed(newState));
    m_state = &STATE_DESCRIPTORS.at(static_cast<size_t>(newState));
    if (newState == DoIPServerState::Closed) {
        closeConnection(reason);
    }

}

DoIPDiagnosticAck DoIPDefaultConnection::notifyDiagnosticMessage(const DoIPMessage &msg) {
    if (m_serverModel->onDiagnosticMessage) {
        return m_serverModel->onDiagnosticMessage(*this, msg);
    }
    return std::nullopt;
}

void DoIPDefaultConnection::notifyConnectionClosed(DoIPCloseReason reason) {
    if (m_serverModel->onCloseConnection) {
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
    m_stateMachine.processEvent(DoIPServerEvent::DiagnosticMessageReceivedDownstream, response);
}

void DoIPDefaultConnection::notifyDownstreamResponseReceived(const DoIPMessage &request, const DoIPMessage &response) {
    if (m_serverModel->onDownstreamResponse) {
        m_serverModel->onDownstreamResponse(*this, request, response);
    }
}

} // namespace doip
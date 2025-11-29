#include "DoIPDefaultConnection.h"
#include "Logger.h"

namespace doip {

DoIPDefaultConnection::DoIPDefaultConnection(UniqueServerModelPtr model)
    : m_stateMachine(*this), m_serverModel(std::move(model)) {}

ssize_t DoIPDefaultConnection::sendProtocolMessage(const DoIPMessage &msg) {
    DOIP_LOG_INFO("Default connection: Sending protocol message: {}", fmt::streamed(msg));
    return static_cast<ssize_t>(msg.size()); // Simulate sending by returning the message size
}

void DoIPDefaultConnection::closeConnection(DoIPCloseReason reason) {
    DOIP_LOG_INFO("Default connection: Closing connection, reason: {}", fmt::streamed(reason));
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
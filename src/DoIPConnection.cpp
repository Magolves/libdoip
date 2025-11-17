#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"

#include <iomanip>
#include <iostream>

using namespace doip;

DoIPConnection::DoIPConnection(int tcpSocket, const DoIPAddress &gatewayAddress) : m_tcpSocket(tcpSocket),
                                                                                   m_gatewayAddress(gatewayAddress),
                                                                                   m_stateMachine([this]() { closeSocket(); }) {
}

/**
 * Closes the connection by closing the sockets
 */
void DoIPConnection::aliveCheckTimeout() {
    DOIP_LOG_ERROR("Alive Check Timeout. Close Connection");
    closeSocket();
}

/*
 * Closes the socket for this server
 */
void DoIPConnection::closeSocket() {
    m_tcpSocket = 0;
    if (m_close_connection)
        m_close_connection();
}

/*
 * Receives a message from the client and calls reactToReceivedTcpMessage method
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
int DoIPConnection::receiveTcpMessage() {
    DOIP_LOG_INFO("Waiting for DoIP Header...");
    uint8_t genericHeader[DOIP_HEADER_SIZE];
    unsigned int readBytes = receiveFixedNumberOfBytesFromTCP(genericHeader, DOIP_HEADER_SIZE);
    if (readBytes == DOIP_HEADER_SIZE && !m_aliveCheckTimer.hasTimeout()) {
        DOIP_LOG_INFO("Received DoIP Header.");

        auto optHeader = DoIPMessage::tryParseHeader(genericHeader, DOIP_HEADER_SIZE);
        if (!optHeader.has_value()) {
            auto sentBytes = sendNegativeAck(DoIPNegativeAck::IncorrectPatternFormat);
            closeSocket();
            return sentBytes;
        }

        auto plType = optHeader->first;
        auto payloadLength = optHeader->second;

        DOIP_LOG_INFO("Payload Type: {}, length: {} ", fmt::streamed(plType), payloadLength);

        if (payloadLength > 0) {
            DOIP_LOG_DEBUG("Waiting for {} bytes of payload...", payloadLength);
            unsigned int receivedPayloadBytes = receiveFixedNumberOfBytesFromTCP(m_receiveBuf.data(), payloadLength);
            if (receivedPayloadBytes < payloadLength) {
                DOIP_LOG_ERROR("DoIP message completely incomplete");
                auto sentBytes = sendNegativeAck(DoIPNegativeAck::InvalidPayloadLength);
                closeSocket();
                return sentBytes;
            }

            DoIPMessage msg = DoIPMessage(plType, m_receiveBuf.data(), receivedPayloadBytes);
            DOIP_LOG_INFO("RX: {}", fmt::streamed(msg));
        }

        // if alive check timouts should be possible, reset timer when message received
        if (m_aliveCheckTimer.isActive()) {
            m_aliveCheckTimer.resetTimer();
        }

        DoIPMessage message(plType, m_receiveBuf.data(), payloadLength);
        int sentBytes = reactOnReceivedTcpMessage(message);

        return sentBytes;
    } else {
        closeSocket();
        return 0;
    }
    return -1;
}

/**
 * Receive exactly payloadLength bytes from the TCP stream and put them into receivedData.
 * The method blocks until receivedData bytes are received or the socket is closed.
 *
 * The parameter receivedData needs to point to a readily allocated array with
 * at least payloadLength items.
 *
 * @return number of bytes received
 */
size_t DoIPConnection::receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength) {
    size_t payloadPos = 0;
    size_t remainingPayload = payloadLength;

    while (remainingPayload > 0) {
        ssize_t result = recv(m_tcpSocket, &receivedData[payloadPos], remainingPayload, 0);
        if (result <= 0) {
            return payloadPos;
        }
        size_t readBytes = static_cast<size_t>(result);
        payloadPos += readBytes;
        remainingPayload -= readBytes;
    }

    return payloadPos;
}
int DoIPConnection::reactOnReceivedTcpMessage(const DoIPMessage &message) {
    // Process the received DoIP message
    ssize_t sentBytes = -1;
    switch (message.getPayloadType()) {
    case DoIPPayloadType::NegativeAck: {
        // should not happen, already handled in receiveTcpMessage
        break;
    }

    case DoIPPayloadType::AliveCheckResponse: {
        return 0;
    }

    case DoIPPayloadType::AliveCheckRequest: {
        // Handle Alive Check Request
        DoIPMessage response = message::makeAliveCheckResponse(m_gatewayAddress);
        sentBytes = sendMessage(response.data(), response.size());
        break;
    }

    case DoIPPayloadType::RoutingActivationRequest: {
        auto optSourceAddress = message.getSourceAddress();
        if (!optSourceAddress.has_value()) {
            DOIP_LOG_WARN("No address in routing request: {}", fmt::streamed(message));
            sentBytes = sendNegativeAck(DoIPNegativeAck::InvalidPayloadLength);
            closeSocket();
            return sentBytes;
        }

        if (!m_aliveCheckTimer.isActive()) {
            m_aliveCheckTimer.setCloseConnectionHandler(std::bind(&DoIPConnection::aliveCheckTimeout, this));
            m_aliveCheckTimer.startTimer();
        }

        m_routedClientAddress = optSourceAddress.value();
        DoIPMessage response = message::makeRoutingActivationResponse(message, m_gatewayAddress);
        sentBytes = sendMessage(response.data(), response.size());

        break;
    }
    case DoIPPayloadType::DiagnosticMessage: {
        auto optTargetAddress = message.getTargetAddress();
        if (!optTargetAddress.has_value()) {
            sentBytes = sendNegativeAck(DoIPNegativeAck::InvalidPayloadLength);
            break;
        }

        DoIPAddress ta = optTargetAddress.value();
        DoIPDiagnosticAck ackResponse = std::nullopt;
        if (m_notify_application && m_notify_application(ta)) {
            ackResponse = handleDiagnosticMessage(m_diag_callback, m_routedClientAddress, message.getPayload().first, message.getPayloadSize());
        }

        if (ackResponse.has_value()) {
            sendDiagnosticNegativeAck(ta, ackResponse.value());
        } else {
            sendDiagnosticAck(ta);
        }

        // continue here

        break;
    }

    default: {
        DOIP_LOG_ERROR("Received message with unhandled payload type: {}", static_cast<uint8_t>(message.getPayloadType()));
        return -1;
    }
    }

    return sentBytes;
}

void DoIPConnection::triggerDisconnection() {
    DOIP_LOG_INFO("Application requested to disconnect Client from Server");
    closeSocket();
}

/**
 * Sends a message back to the connected client
 * @param message           contains generic header and payload specific content
 * @param messageLength     length of the complete message
 * @return                  number of bytes written is returned,
 *                          or -1 if error occurred
 */
ssize_t DoIPConnection::sendMessage(const uint8_t *message, size_t messageLength) {
    ssize_t result = write(m_tcpSocket, message, messageLength);
    return result;
}

/**
 * Sets the time in seconds after which a alive check timeout occurs.
 * Alive check timeouts can be deactivated when setting the seconds to 0
 * @param seconds   time after which alive check timeout occurs
 */
void DoIPConnection::setGeneralInactivityTime(uint16_t seconds) {
    if (seconds > 0) {
        m_aliveCheckTimer.setTimer(seconds);
    } else {
        m_aliveCheckTimer.setDisabled(true);
    }
}

/*
 * Send diagnostic message payload to the client
 * @param sourceAddress   logical source address (i.e. address of this server)
 * @param value     received payload
 * @param length    length of received payload
 */
void DoIPConnection::sendDiagnosticPayload(const DoIPAddress &sourceAddress, const ByteArray &payload) {

    DOIP_LOG_INFO("Sending diagnostic data: {}", fmt::streamed(payload));

    // uint8_t* message = createDiagnosticMessage(sourceAddress, m_routedClientAddress, data, length);
    DoIPMessage msg = message::makeDiagnosticMessage(sourceAddress, m_routedClientAddress, payload);
    sendMessage(msg.data(), msg.size());
}

/*
 * Set the callback function for this doip server instance
 * @dc      Callback which sends the data of a diagnostic message to the application
 * @dmn     Callback which notifies the application of receiving a diagnostic message
 * @ccb     Callback for application function when the library closes the connection
 */
void DoIPConnection::setCallback(DiagnosticCallback dc, DiagnosticMessageNotification dmn, CloseConnectionCallback ccb) {
    m_diag_callback = dc;
    m_notify_application = dmn;
    m_close_connection = ccb;
}

void DoIPConnection::sendDiagnosticAck(const DoIPAddress &sourceAddress) {
    DoIPMessage msg = message::makeDiagnosticPositiveResponse(sourceAddress, m_routedClientAddress, {});
    sendMessage(msg.data(), msg.size());
}

void DoIPConnection::sendDiagnosticNegativeAck(const DoIPAddress &sourceAddress, DoIPNegativeDiagnosticAck ackCode) {
    DoIPMessage msg = message::makeDiagnosticNegativeResponse(sourceAddress, m_routedClientAddress, ackCode, {});
    sendMessage(msg.data(), msg.size());
}

/**
 * Prepares a generic header nack and sends it to the client
 * @param ackCode       NACK-Code which will be included in the message
 * @return              amount of bytes sended to the client
 */
int DoIPConnection::sendNegativeAck(DoIPNegativeAck ackCode) {
    DoIPMessage msg = message::makeNegativeAckMessage(ackCode);
    DOIP_LOG_WARN("NACK: {}", fmt::streamed(msg));
    int sentBytes = sendMessage(msg.data(), msg.size());
    return sentBytes;
}

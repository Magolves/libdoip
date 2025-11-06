#include "DoIPConnection.h"
#include "DoIPMessage.h"

#include <iostream>
#include <iomanip>

using namespace doip;

/**
 * Closes the connection by closing the sockets
 */
void DoIPConnection::aliveCheckTimeout() {
    std::cout << "Alive Check Timeout. Close Connection" << '\n';
    closeSocket();
    m_close_connection();
}

/*
 * Closes the socket for this server
 */
void DoIPConnection::closeSocket() {
    close(m_tcpSocket);
    m_tcpSocket = 0;
}

/*
 * Receives a message from the client and calls reactToReceivedTcpMessage method
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
int DoIPConnection::receiveTcpMessage() {
    std::cout << "Waiting for DoIP Header..." << '\n';
    uint8_t genericHeader[DoIPMessage::DOIP_HEADER_SIZE];
    unsigned int readBytes = receiveFixedNumberOfBytesFromTCP(DoIPMessage::DOIP_HEADER_SIZE, genericHeader);
    if(readBytes == DoIPMessage::DOIP_HEADER_SIZE && !m_aliveCheckTimer.hasTimeout()) {
        std::cout << "Received DoIP Header." << '\n';
        GenericHeaderAction doipHeaderAction = parseGenericHeader(genericHeader, DoIPMessage::DOIP_HEADER_SIZE);

        uint8_t *payload = nullptr;
        if(doipHeaderAction.payloadLength > 0) {
            std::cout << "Waiting for " << doipHeaderAction.payloadLength << " bytes of payload..." << '\n';
            payload = new uint8_t[doipHeaderAction.payloadLength];
            unsigned int receivedPayloadBytes = receiveFixedNumberOfBytesFromTCP(doipHeaderAction.payloadLength, payload);
            if(receivedPayloadBytes != doipHeaderAction.payloadLength) {
                closeSocket();
                return 0;
            }
            std::cout << "DoIP message completely received" << '\n';
        }

        //if alive check timouts should be possible, reset timer when message received
        if(m_aliveCheckTimer.isActive()) {
            m_aliveCheckTimer.resetTimer();
        }

        int sentBytes = reactOnReceivedTcpMessage(doipHeaderAction, doipHeaderAction.payloadLength, payload);

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
size_t DoIPConnection::receiveFixedNumberOfBytesFromTCP(size_t payloadLength, uint8_t *receivedData) {
    size_t payloadPos = 0;
    size_t remainingPayload = payloadLength;

    while(remainingPayload > 0) {
        ssize_t result = recv(m_tcpSocket, &receivedData[payloadPos], remainingPayload, 0);
        if(result <= 0) {
            return payloadPos;
        }
        size_t readBytes = static_cast<size_t>(result);
        payloadPos += readBytes;
        remainingPayload -= readBytes;
    }

    return payloadPos;
}

/*
 * Receives a message from the client and determine how to process the message
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
int DoIPConnection::reactOnReceivedTcpMessage(GenericHeaderAction action, unsigned long payloadLength, uint8_t *payload) {

    std::cout << "processing DoIP message..." << '\n';
    int sentBytes;
    switch(action.type) {
        case PayloadType::NEGATIVEACK: {
            //send NACK
            sentBytes = sendNegativeAck(action.value);

            if(action.value == _IncorrectPatternFormatCode ||
                    action.value == _InvalidPayloadLengthCode) {
                closeSocket();
                return -1;
            }

            return sentBytes;
        }

        case PayloadType::ROUTINGACTIVATIONREQUEST: {
            //start routing activation handler with the received message
            uint8_t result = parseRoutingActivation(payload);
            DoIPAddress clientAddress(payload[0], payload[1]);

            uint8_t* message = createRoutingActivationResponse(m_logicalGatewayAddress, clientAddress, result);
            sentBytes = sendMessage(message, DoIPMessage::DOIP_HEADER_SIZE + _ActivationResponseLength);

            if(result == _UnknownSourceAddressCode || result == _UnsupportedRoutingTypeCode) {
                closeSocket();
                return -1;
            } else {
                //Routing Activation Request was successfull, save address of the client
                m_routedClientAddress.update(payload[0], payload[1]);

                //start alive check timer
                if(!m_aliveCheckTimer.isActive()) {
                    m_aliveCheckTimer.setCloseConnectionHandler(std::bind(&DoIPConnection::aliveCheckTimeout,this));
                    m_aliveCheckTimer.startTimer();
                }
            }

            return sentBytes;
        }

        case PayloadType::ALIVECHECKRESPONSE: {
            return 0;
        }

        case PayloadType::DIAGNOSTICMESSAGE: {

            DoIPAddress target_address(payload[2], payload[3]);
            bool ack = m_notify_application && m_notify_application(target_address);

            if(ack)
                handleDiagnosticMessage(m_diag_callback, m_routedClientAddress, payload, payloadLength);

            break;
        }

        default: {
            std::cerr << "Received message with unhandled payload type: " << +static_cast<uint8_t>(action.type) << '\n';
            return -1;
        }
    }
    return -1;
}

void DoIPConnection::triggerDisconnection() {
    std::cout << "Application requested to disconnect Client from Server" << '\n';
    closeSocket();
}

/**
 * Sends a message back to the connected client
 * @param message           contains generic header and payload specific content
 * @param messageLength     length of the complete message
 * @return                  number of bytes written is returned,
 *                          or -1 if error occurred
 */
ssize_t DoIPConnection::sendMessage(uint8_t* message, size_t messageLength) {
    ssize_t result = write(m_tcpSocket, message, messageLength);
    return result;
}

ssize_t DoIPConnection::sendMessage(const ByteArray& message) {
    ssize_t result = write(m_tcpSocket, message.data(), message.size());
    return result;
}

/**
 * Sets the time in seconds after which a alive check timeout occurs.
 * Alive check timeouts can be deactivated when setting the seconds to 0
 * @param seconds   time after which alive check timeout occurs
 */
void DoIPConnection::setGeneralInactivityTime(uint16_t seconds) {
    if(seconds > 0) {
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
void DoIPConnection::sendDiagnosticPayload(const DoIPAddress& sourceAddress, const ByteArray& payload) {

    std::cout << "Sending diagnostic data: ";
    for(auto byte: payload) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << byte << " ";
    }
    std::cout << '\n';

    //uint8_t* message = createDiagnosticMessage(sourceAddress, m_routedClientAddress, data, length);
    ByteArray message = DoIPMessage::makeDiagnosticMessage(sourceAddress, m_routedClientAddress, payload).toBytes();

    sendMessage(message);
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

void DoIPConnection::sendDiagnosticAck(const DoIPAddress& sourceAddress) {
    ByteArray message = DoIPMessage::makeDiagnosticPositiveResponse(sourceAddress, m_routedClientAddress, {}).toBytes();
    sendMessage(message);
}

void DoIPConnection::sendDiagnosticNegativeAck(const DoIPAddress& sourceAddress, DoIPNegativeDiagnosticAck ackCode) {
    ByteArray message = DoIPMessage::makeDiagnosticNegativeResponse(sourceAddress, m_routedClientAddress, ackCode, {}).toBytes();
    sendMessage(message);
}

/**
 * Prepares a generic header nack and sends it to the client
 * @param ackCode       NACK-Code which will be included in the message
 * @return              amount of bytes sended to the client
 */
int DoIPConnection::sendNegativeAck(uint8_t ackCode) {
    uint8_t* message = createGenericHeader(PayloadType::NEGATIVEACK, _NACKLength);
    message[8] = ackCode;
    int sendedBytes = sendMessage(message, DoIPMessage::DOIP_HEADER_SIZE + _NACKLength);
    return sendedBytes;
}

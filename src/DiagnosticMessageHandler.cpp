#include "DiagnosticMessageHandler.h"
#include <cstring>
#include <iostream>

namespace doip {

/**
 * Checks if a received Diagnostic Message is valid
 * @param callback                   callback which will be called with the user data
 * @param sourceAddress		currently registered source address on the socket
 * @param data			message which was received
 * @param diagMessageLength     length of the diagnostic message
 */
uint8_t parseDiagnosticMessage(DiagnosticCallback callback, const DoIPAddress &sourceAddress,
                               const uint8_t *data, size_t diagMessageLength) {
    std::cout << "parse Diagnostic Message" << '\n';
    if (diagMessageLength >= _DiagnosticMessageMinimumLength) {
        // Check if the received SA is registered on the socket
        if (data[0] != sourceAddress.hsb() || data[1] != sourceAddress.lsb()) {
            // SA of received message is not registered on this TCP_DATA socket
            return _InvalidSourceAddressCode;
        }

        std::cout << "source address valid" << '\n';
        // Pass the diagnostic message to the target network/transport layer
        DoIPAddress target_address(data, 2);

        size_t cb_message_length = diagMessageLength - _DiagnosticMessageMinimumLength;
        uint8_t *cb_message = new uint8_t[cb_message_length];

        for (size_t i = _DiagnosticMessageMinimumLength; i < diagMessageLength; i++) {
            cb_message[i - _DiagnosticMessageMinimumLength] = data[i];
        }

        callback(target_address, cb_message, cb_message_length);

        // return positive ack code
        return _ValidDiagnosticMessageCode;
    }
    return _UnknownTargetAddressCode;
}

/**
 * Creates a diagnostic message positive/negative acknowledgment message
 * @param ackType                  defines positive/negative acknowledge type
 * @param sourceAddress		logical address of the receiver of the previous diagnostic message
 * @param targetAddress		logical address of the sender of the previous diagnostic message
 * @param responseCode		positive or negative acknowledge code
 * @return pointer to the created diagnostic message acknowledge
 */
uint8_t *createDiagnosticACK(bool ackType, const DoIPAddress &sourceAddress,
                             const DoIPAddress &targetAddress, uint8_t responseCode) {

    PayloadType type;
    if (ackType)
        type = PayloadType::DIAGNOSTICPOSITIVEACK;
    else
        type = PayloadType::DIAGNOSTICNEGATIVEACK;

    uint8_t *message = createGenericHeader(type, _DiagnosticPositiveACKLength);

    // add source address to the message
    message[8] = sourceAddress.lsb();
    message[9] = sourceAddress.hsb();

    // add target address to the message
    message[10] = targetAddress.lsb();
    message[11] = targetAddress.hsb();

    // add positive or negative acknowledge code to the message
    message[12] = responseCode;

    return message;
}

/**
 * Creates a complete diagnostic message
 * @param sourceAddress		logical address of the sender of a diagnostic message
 * @param targetAddress		logical address of the receiver of a diagnostic message
 * @param userData		actual diagnostic data
 * @param userDataLength	length of diagnostic data
 */
uint8_t *createDiagnosticMessage(const DoIPAddress &sourceAddress, const DoIPAddress &targetAddress,
                                 uint8_t *userData, size_t userDataLength) {

    uint8_t *message = createGenericHeader(PayloadType::DIAGNOSTICMESSAGE, _DiagnosticMessageMinimumLength + userDataLength);

    // add source address to the message
    message[8] = sourceAddress.lsb();
    message[9] = sourceAddress.hsb();

    // add target address to the message
    message[10] = targetAddress.lsb();
    message[11] = targetAddress.hsb();

    // add userdata to the message
    for (size_t i = 0; i < userDataLength; i++) {
        message[12 + i] = userData[i];
    }

    return message;
}

} // namespace doip
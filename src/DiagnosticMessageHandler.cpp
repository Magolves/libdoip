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

} // namespace doip
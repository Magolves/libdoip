#include "DiagnosticMessageHandler.h"
#include "DoIPMessage.h"

#include <cstring>
#include <iostream>

namespace doip {

/* Thanks to the brain-dead decision to reserve the zero in DoIPNegativeDiagnosticAck, we have to use std::optional to indicate 'ok' */
DoIPDiagnosticAck handleDiagnosticMessage(DiagnosticCallback callback, const DoIPAddress &sourceAddress, const uint8_t *data, size_t length) {
    std::cout << "parse Diagnostic Message" << '\n';

    auto optMsg = DoIPMessage::tryParse(data, length);
    if (optMsg.has_value() == false) {
        std::cout << "Could not parse DoIP message" << '\n';
        return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
    }

    auto msg = optMsg.value();
    if (msg.getPayloadType() != DoIPPayloadType::DiagnosticMessage) {
        std::cout << "Received message is not a Diagnostic Message" << '\n';
        return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
    }

    auto optSourceAddress = msg.getSourceAddress();
    if (optSourceAddress != sourceAddress) {
        std::cout << "Source address of received message does not match registered source address" << '\n';
        return DoIPNegativeDiagnosticAck::InvalidSourceAddress;
    }

    auto optTargetAddress = msg.getTargetAddress();
    if (!optTargetAddress.has_value()) {
        std::cout << "Could not extract target address from Diagnostic Message" << '\n';
        return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
    }

    if (callback) {
        auto [payload_ptr, payload_size] = msg.getPayload();
        callback(optTargetAddress.value(), payload_ptr, payload_size);

    }
    // return positive ack code
    return std::nullopt;
}

} // namespace doip
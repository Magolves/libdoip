#ifndef DOIPNEGATIVEDIAGNOSTICACK_H
#define DOIPNEGATIVEDIAGNOSTICACK_H

#include <stdint.h>
#include <optional>
#include <iostream>
#include <iomanip>

namespace doip {

// Table 26
enum class DoIPNegativeDiagnosticAck : uint8_t {
    // 0 and 1: reserved
    InvalidSourceAddress = 2,
    UnknownTargetAddress = 3,
    DiagnosticMessageTooLarge = 4,
    OutOfMemory = 5,
    TargetUnreachable = 6,
    UnknownNetwork = 7,
    TransportProtocolError = 8, // also use if other error codes do not apply
    TargetBusy = 9,
};

/**
 * @brief Alias for diagnostic acknowledgment type.
 *
 * This type represents either a successful acknowledgment (std::nullopt) or
 * a negative acknowledgment (DoIPNegativeDiagnosticAck).
 * This is to circumvent the reserved value '0' in DoIPNegativeDiagnosticAck.
 */
using DoIPDiagnosticAck = std::optional<DoIPNegativeDiagnosticAck>;

/**
 * @brief Stream output operator for DoIPNegativeDiagnosticAck
 *
 * @param os the output stream
 * @param nack the negative acknowledgment code
 * @return std::ostream& the output stream
 */
inline std::ostream& operator<<(std::ostream& os, doip::DoIPNegativeDiagnosticAck nack) {
    const char* name = nullptr;
    switch (nack) {
        case doip::DoIPNegativeDiagnosticAck::InvalidSourceAddress:
            name = "InvalidSourceAddress";
            break;
        case doip::DoIPNegativeDiagnosticAck::UnknownTargetAddress:
            name = "UnknownTargetAddress";
            break;
        case doip::DoIPNegativeDiagnosticAck::DiagnosticMessageTooLarge:
            name = "DiagnosticMessageTooLarge";
            break;
        case doip::DoIPNegativeDiagnosticAck::OutOfMemory:
            name = "OutOfMemory";
            break;
        case doip::DoIPNegativeDiagnosticAck::TargetUnreachable:
            name = "TargetUnreachable";
            break;
        case doip::DoIPNegativeDiagnosticAck::UnknownNetwork:
            name = "UnknownNetwork";
            break;
        case doip::DoIPNegativeDiagnosticAck::TransportProtocolError:
            name = "TransportProtocolError";
            break;
        case doip::DoIPNegativeDiagnosticAck::TargetBusy:
            name = "TargetBusy";
            break;
        default:
            name = "Unknown";
            break;
    }
    os << name << " (0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<unsigned int>(static_cast<uint8_t>(nack)) << std::dec << ")";

    return os;
}

/**
 * @brief Stream output operator for DoIPNegativeDiagnosticAck
 *
 * @param os the output stream
 * @param ack the negative acknowledgment code
 * @return std::ostream& the output stream
 */
inline std::ostream& operator<<(std::ostream& os, doip::DoIPDiagnosticAck ack) {
    if (!ack.has_value()) {
        os << "PositiveAck (0x00)";
        return os;
    }

    return os << ack.value();
}

}
#endif  /* DOIPNEGATIVEDIAGNOSTICACK_H */

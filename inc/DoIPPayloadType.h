#ifndef DOIPPAYLOADTYPE_H
#define DOIPPAYLOADTYPE_H

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <optional>

namespace doip {

// Table 17
enum class DoIPPayloadType : uint16_t  {
    NegativeAck = 0x0000,
    VehicleIdentificationRequest = 0x0001,
    VehicleIdentificationRequestWithEid = 0x0002,
    VehicleIdentificationRequestWithVin = 0x0003,
    VehicleIdentificationResponse = 0x0004, // aka Vehicle announcement message
    RoutingActivationRequest = 0x0005,
    RoutingActivationResponse = 0x0006,
    AliveCheckRequest = 0x0007,
    AliveCheckResponse = 0x0008,
    // 0x0009 - 0x4000: reserved
    EntityStatusRequest = 0x4001,
    EntityStatusResponse = 0x4002,
    DiagnosticPowerModeRequest = 0x4003,
    DiagnosticPowerModeResponse = 0x4004,
    // 0x4005 - 0x8000: reserved
    DiagnosticMessage = 0x8001,
    DiagnosticMessageAck = 0x8002, // positive response
    DiagnosticMessageNegativeAck = 0x8003, // negative response
    PeriodicDiagnosticMessage = 0x8004
    // 0x8005 - 0x8FFF : reserved
    // 0x9000 - 0x9FFF : subnet protocol
    // 0xA000 - 0xEFFF : reserved
    // 0xF000 - 0xFFFF : reserved for manufacturer
};

/**
 * @brief Validates if a uint16_t value represents a valid DoIPPayloadType
 * @param value The value to validate
 * @return true if valid, false otherwise
 */
constexpr bool isValidPayloadType(uint16_t value) noexcept {
    switch (value) {
        case 0x0000: // NegativeAck
        case 0x0001: // VehicleIdentificationRequest
        case 0x0002: // VehicleIdentificationRequestWithEid
        case 0x0003: // VehicleIdentificationRequestWithVin
        case 0x0004: // VehicleIdentificationResponse
        case 0x0005: // RoutingActivationRequest
        case 0x0006: // RoutingActivationResponse
        case 0x0007: // AliveCheckRequest
        case 0x0008: // AliveCheckResponse
        case 0x4001: // EntityStatusRequest
        case 0x4002: // EntityStatusResponse
        case 0x4003: // DiagnosticPowerModeRequest
        case 0x4004: // DiagnosticPowerModeResponse
        case 0x8001: // DiagnosticMessage
        case 0x8002: // DiagnosticMessageAck
        case 0x8003: // DiagnosticMessageNegativeAck
        case 0x8004: // PeriodicDiagnosticMessage
            return true;
        default:
            return false;
    }
}

/**
 * @brief Safely converts uint16_t to DoIPPayloadType with validation
 * @param value The value to convert
 * @return Optional containing the payload type if valid, nullopt otherwise
 */
constexpr std::optional<DoIPPayloadType> toPayloadType(uint16_t value) noexcept {
    if (isValidPayloadType(value)) {
        return static_cast<DoIPPayloadType>(value);
    }
    return std::nullopt;
}

constexpr std::optional<DoIPPayloadType> toPayloadType(uint8_t hsb, uint8_t lsb) noexcept {
    return toPayloadType(static_cast<uint16_t>((hsb << 8) | lsb));
}

/**
 * @brief Stream operator for DoIPPayloadType enum
 *
 * Prints the payload type name and its hex value.
 * Example: "DiagnosticMessage (0x8001)"
 *
 * @param os Output stream
 * @param type Payload type to print
 * @return std::ostream& Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, DoIPPayloadType type) {
    const char* name = nullptr;

    switch (type) {
        case DoIPPayloadType::NegativeAck:
            name = "NegativeAck";
            break;
        case DoIPPayloadType::VehicleIdentificationRequest:
            name = "VehicleIdentificationRequest";
            break;
        case DoIPPayloadType::VehicleIdentificationRequestWithEid:
            name = "VehicleIdentificationRequestWithEid";
            break;
        case DoIPPayloadType::VehicleIdentificationRequestWithVin:
            name = "VehicleIdentificationRequestWithVin";
            break;
        case DoIPPayloadType::VehicleIdentificationResponse:
            name = "VehicleIdentificationResponse";
            break;
        case DoIPPayloadType::RoutingActivationRequest:
            name = "RoutingActivationRequest";
            break;
        case DoIPPayloadType::RoutingActivationResponse:
            name = "RoutingActivationResponse";
            break;
        case DoIPPayloadType::AliveCheckRequest:
            name = "AliveCheckRequest";
            break;
        case DoIPPayloadType::AliveCheckResponse:
            name = "AliveCheckResponse";
            break;
        case DoIPPayloadType::EntityStatusRequest:
            name = "EntityStatusRequest";
            break;
        case DoIPPayloadType::EntityStatusResponse:
            name = "EntityStatusResponse";
            break;
        case DoIPPayloadType::DiagnosticPowerModeRequest:
            name = "DiagnosticPowerModeRequest";
            break;
        case DoIPPayloadType::DiagnosticPowerModeResponse:
            name = "DiagnosticPowerModeResponse";
            break;
        case DoIPPayloadType::DiagnosticMessage:
            name = "DiagnosticMessage";
            break;
        case DoIPPayloadType::DiagnosticMessageAck:
            name = "DiagnosticMessageAck";
            break;
        case DoIPPayloadType::DiagnosticMessageNegativeAck:
            name = "DiagnosticMessageNegativeAck";
            break;
        case DoIPPayloadType::PeriodicDiagnosticMessage:
            name = "PeriodicDiagnosticMessage";
            break;
        default:
            name = "Unknown";
            break;
    }

    os << name << " (0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
       << static_cast<uint16_t>(type) << std::dec << ")";

    return os;
}

} // namespace doip

#endif  /* DOIPPAYLOADTYPE_H */

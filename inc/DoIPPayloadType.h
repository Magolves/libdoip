#ifndef DOIPPAYLOADTYPE_H
#define DOIPPAYLOADTYPE_H

#include <stdint.h>
#include <iostream>
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
    return toPayloadType(hsb << 8 | lsb);
}


} // namespace doip

#endif  /* DOIPPAYLOADTYPE_H */

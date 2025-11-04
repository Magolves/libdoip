#ifndef DOIPPAYLOADTYPE_H
#define DOIPPAYLOADTYPE_H

#include <stdint.h>
#include <iostream>

namespace doip {

enum class DoIPPayloadType : uint16_t  {
    GenericHeaderNack = 0x0000,
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
    DiagnosticMessageAck = 0x8001, // positive response
    DiagnosticMessageNack = 0x8003, // negative response
    PeriodicDiagnosticMessage = 0x8004
};

} // namespace doip

#endif  /* DOIPPAYLOADTYPE_H */

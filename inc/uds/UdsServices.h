#ifndef UDSSERVICES_H
#define UDSSERVICES_H

#include <type_traits>
#include <utility>
#include <array>
#include <cstdint>


namespace doip::uds {
enum class UdsService : uint8_t {
    DiagnosticSessionControl = 0x10,
    ECUReset = 0x11,
    SecurityAccess = 0x27,
    CommunicationControl = 0x28,
    TesterPresent = 0x3E,
    AccessTimingParameters = 0x83,
    SecuredDataTransmission = 0x84,
    ControlDTCSetting = 0x85,
    ResponseOnEvent = 0x86,
    LinkControl = 0x87,
    ReadDataByIdentifier = 0x22,
    ReadMemoryByAddress = 0x23,
    ReadScalingDataByIdentifier = 0x24,
    ReadDataByPeriodicIdentifier = 0x2A,
    DynamicallyDefineDataIdentifier = 0x2C,
    WriteDataByIdentifier = 0x2E,
    WriteMemoryByAddress = 0x3D,
    ClearDiagnosticInformation = 0x14,
    ReadDTCInformation = 0x19,
};





} // namespace doip::uds

#endif /* UDSSERVICES_H */

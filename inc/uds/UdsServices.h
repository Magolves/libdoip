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
    RequestDownload = 0x34,
    TransferData = 0x36,
    RequestTransferExit = 0x37,
};

using uds_length = uint16_t;
struct UdsServiceDescriptor {
    UdsService service;
    uds_length minReqLength;
    uds_length maxReqLength;
    uds_length minRspLength;
    uds_length maxRspLength;
};

constexpr uds_length MAX_UDS_MESSAGE_LENGTH = 4095;

constexpr std::array<UdsServiceDescriptor, 22> UDS_SERVICE_DESCRIPTORS = {{
    { UdsService::DiagnosticSessionControl, 2, 2, 6, 6 },
    { UdsService::ECUReset, 2, 2, 2, 2 },
    { UdsService::SecurityAccess, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::CommunicationControl, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::TesterPresent, 2, 2, 2, 2 },
    { UdsService::AccessTimingParameters, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::SecuredDataTransmission, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ControlDTCSetting, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ResponseOnEvent, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::LinkControl, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ReadDataByIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 4, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ReadMemoryByAddress, 4, MAX_UDS_MESSAGE_LENGTH, 4, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ReadScalingDataByIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ReadDataByPeriodicIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::DynamicallyDefineDataIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::WriteDataByIdentifier, 4, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::WriteMemoryByAddress, 4, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ClearDiagnosticInformation, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::ReadDTCInformation, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::RequestDownload, 6, MAX_UDS_MESSAGE_LENGTH, 5, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::TransferData, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH },
    { UdsService::RequestTransferExit, 2, 2, 2, 2 }
}};

/**
 * @brief Find service descriptor by service ID
 *
 * @param sid the UDS service ID
 * @return const UdsServiceDescriptor* the service descriptor or nullptr if not found
 */
inline const UdsServiceDescriptor* findServiceDescriptor(UdsService sid) {
    auto it = std::find_if(UDS_SERVICE_DESCRIPTORS.begin(), UDS_SERVICE_DESCRIPTORS.end(),
                               [sid](const UdsServiceDescriptor& desc) {
                                   return desc.service == sid;
                               });
    if (it != UDS_SERVICE_DESCRIPTORS.end()) {
        return &(*it);
    }
    return nullptr;
}

} // namespace doip::uds

#endif /* UDSSERVICES_H */

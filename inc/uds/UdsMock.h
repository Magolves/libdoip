#ifndef UDSMOCK_H
#define UDSMOCK_H

#include <functional>
#include <unordered_map>

#include "DoIPMessage.h"
#include "UdsResponseCode.h"

using namespace doip;

namespace doip::uds {

using UdsResponse = std::pair<UdsResponseCode, ByteArray>;
using UdsServiceHandler = std::function<UdsResponse(const ByteArray &request)>;

struct UdsServiceDescriptor {
    uint8_t serviceId;
    size_t requestMinLength;
    size_t responseMinLength;
    UdsServiceHandler handler;
};

constexpr uint8_t UDS_POSITIVE_RESPONSE_OFFSET = 0x40;

// Lookup table
/*
const std::array<UdsServiceDescriptor, 14> UdsServiceLookupTable = {{
    {0x10, 2, 2, [](const ByteArray& request) { return handleDiagnosticSessionControl(request); }},
    {0x11, 2, 2, [](const ByteArray& request) { return handleECUReset(request); }},
    {0x27, 2, 2, [](const ByteArray& request) { return handleSecurityAccess(request); }},
    {0x28, 2, 2, [](const ByteArray& request) { return handleCommunicationControl(request); }},
    {0x3E, 1, 1, [](const ByteArray& request) { return handleTesterPresent(request); }},
    {0x83, 2, 2, [](const ByteArray& request) { return handleAccessTimingParameters(request); }},
    {0x84, 2, 2, [](const ByteArray& request) { return handleSecuredDataTransmission(request); }},
    {0x85, 2, 2, [](const ByteArray& request) { return handleControlDTCSetting(request); }},
    {0x86, 2, 2, [](const ByteArray& request) { return handleResponseOnEvent(request); }},
    {0x87, 2, 2, [](const ByteArray& request) { return handleLinkControl(request); }},
    {0x22, 3, 2, [](const ByteArray& request) { return handleReadDataByIdentifier(request); }},
    {0x23, 5, 2, [](const ByteArray& request) { return handleReadMemoryByAddress(request); }},
    {0x24, 3, 2, [](const ByteArray& request) { return handleReadScalingDataByIdentifier(request); }},
    {0x2A, 3, 2, [](const ByteArray& request) { return handleReadDataByPeriodicIdentifier(request); }},
    {0x2C, 3, 2, [](const ByteArray& request) { return handleDynamicallyDefineDataIdentifier(request); }},
    {0x2E, 3, 2, [](const ByteArray& request) { return handleWriteDataByIdentifier(request); }},
    {0x3D, 5, 2, [](const ByteArray& request) { return handleWriteMemoryByAddress(request); }},
    {0x14, 2, 2, [](const ByteArray& request) { return handleClearDiagnosticInformation(request); }},
    {0x19, 2, 2, [](const ByteArray& request) { return handleReadDTCInformation(request); }},
}};
*/

//static_assert(UdsServiceLookupTable.size() == 14, "Update the lookup table if new services are added!");

/**
 * @brief Mock UDS service class for testing purposes
 *
 * This class simulates UDS service behavior for unit tests.
 */
class UdsMock {
  public:
    explicit UdsMock(const UdsServiceHandler& handler = UdsMock::defaultHandler) : m_serviceHandler(handler) {}

    /**
     * @brief Simulates handling a UDS diagnostic request
     * @param request The incoming UDS request payload
     * @return The simulated UDS response payload
     */
    ByteArray handleDiagnosticRequest(const ByteArray &request) {
        // Simple echo behavior for testing
        auto response = m_serviceHandler(request);
        return makeResponse(request, response.first, response.second);
    }

  private:
    ByteArray makeResponse(const ByteArray &request, UdsResponseCode responseCode = UdsResponseCode::OK, const ByteArray &extraData = {}) {
        if (responseCode != UdsResponseCode::OK) {
            ByteArray negativeResponse;
            negativeResponse.emplace_back(0x7F);                               // Negative response indicator
            negativeResponse.emplace_back(request[0]);                         // Original service ID
            negativeResponse.emplace_back(static_cast<uint8_t>(responseCode)); // NRC
            return negativeResponse;
        }

        ByteArray positiveResponse;
        positiveResponse.emplace_back(static_cast<uint8_t>(request[0] + UDS_POSITIVE_RESPONSE_OFFSET)); // Positive response SID
        positiveResponse.insert(positiveResponse.end(), extraData.begin(), extraData.end());
        return positiveResponse;
    }

    static UdsResponse defaultHandler(const ByteArray &request) {
        (void)request; // Unused
        return {UdsResponseCode::ServiceNotSupported, {}};
    }

    UdsServiceHandler m_serviceHandler;
};

} // namespace doip::uds

#endif /* UDSMOCK_H */

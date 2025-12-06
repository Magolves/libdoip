#ifndef UDSMOCK_H
#define UDSMOCK_H

#include <array>
#include <functional>
#include <memory>
#include <unordered_map>

#include "DoIPMessage.h"
#include "IUdsServiceHandler.h"
#include "LambdaUdsHandler.h"
#include "UdsResponseCode.h"
#include "UdsServices.h"

using namespace doip;

namespace doip::uds {

constexpr uint8_t UDS_POSITIVE_RESPONSE_OFFSET = 0x40;

class UdsMock {
  public:
    UdsMock() = default;

    // Register a handler owning pointer
    void registerService(UdsService serviceId, IUdsServiceHandlerPtr handler) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::move(handler);
    }

    // Register a lambda/function
    void registerService(UdsService serviceId, std::function<UdsResponse(const ByteArray &)> fn) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<LambdaUdsHandler>(std::move(fn));
    }

    // Unregister
    void unregisterService(UdsService serviceId) {
        m_handlers.erase(static_cast<uint8_t>(serviceId));
    }

    // Convenience: clear all
    void clear() { m_handlers.clear(); }

    // --- Typed registration helpers (convenience wrappers) ---
    // Diagnostic Session Control (0x10): handler(sessionType)
    void registerDiagnosticSessionControlHandler(std::function<UdsResponse(uint8_t sessionType)> handler);

    // ECU Reset (0x11): handler(resetType)
    void registerECUResetHandler(std::function<UdsResponse(uint8_t resetType)> handler);

    // Read Data By Identifier (0x22): handler(did, params)
    void registerReadDataByIdentifierHandler(std::function<UdsResponse(uint16_t did)> handler);

    // Write Data By Identifier (0x2E): handler(did, data)
    void registerWriteDataByIdentifierHandler(std::function<UdsResponse(uint16_t did, ByteArray value)> handler);

    // Tester Present (0x3E): handler(subFunction)
    void registerTesterPresentHandler(std::function<UdsResponse(uint8_t subFunction)> handler);

    
    ByteArray handleDiagnosticRequest(const ByteArray &request) {
        if (request.empty())
            return {};
        uint8_t sid = request[0];
        UdsService service = static_cast<UdsService>(sid);

        const UdsServiceDescriptor *desc = findServiceDescriptor(service);
        if (!desc) {
            return makeResponse(request, UdsResponseCode::ServiceNotSupported, {});
        }

        if (request.size() < desc->minReqLength || request.size() > desc->maxReqLength) {
            std::cerr << "UdsMock: Request length " << request.size()
                      << " out of bounds for service 0x" << std::hex <<  static_cast<int>(service) << std::dec
                      << " (expected " << desc->minReqLength << "-" << desc->maxReqLength << ")\n";
            return makeResponse(request, UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, {});
        }

        UdsResponse resp = defaultResponse(request);
        auto it = m_handlers.find(sid);
        if (it != m_handlers.end() && it->second) {
            resp = it->second->handle(request);
        }

        auto rspSize = resp.second.size() + 1; // +1 for the SID
        if (rspSize < desc->minRspLength || rspSize > desc->maxRspLength) {
            std::cerr << "UdsMock: Response length " << resp.second.size()
                      << " out of bounds for service 0x" << std::hex <<  static_cast<int>(service) << std::dec
                      << " (expected " << desc->minRspLength << "-" << desc->maxRspLength << ")\n";
            return makeResponse(request, UdsResponseCode::GeneralProgrammingFailure, {});
        }

        return makeResponse(request, resp.first, resp.second);
    }

    // Register default handlers for all known services.
    // By default these handlers simply return ServiceNotSupported. Tests
    // can register custom handlers afterwards to override behavior.
    void registerDefaultServices() {
        const std::array<UdsService, 19> services = {
            UdsService::DiagnosticSessionControl,
            UdsService::ECUReset,
            UdsService::SecurityAccess,
            UdsService::CommunicationControl,
            UdsService::TesterPresent,
            UdsService::AccessTimingParameters,
            UdsService::SecuredDataTransmission,
            UdsService::ControlDTCSetting,
            UdsService::ResponseOnEvent,
            UdsService::LinkControl,
            UdsService::ReadDataByIdentifier,
            UdsService::ReadMemoryByAddress,
            UdsService::ReadScalingDataByIdentifier,
            UdsService::ReadDataByPeriodicIdentifier,
            UdsService::DynamicallyDefineDataIdentifier,
            UdsService::WriteDataByIdentifier,
            UdsService::WriteMemoryByAddress,
            UdsService::ClearDiagnosticInformation,
            UdsService::ReadDTCInformation,
        };

        for (auto s : services) {
            registerService(s, [](const ByteArray &req) -> UdsResponse {
                (void)req;
                return {UdsResponseCode::ServiceNotSupported, {}};
            });
        }
    }

  private:
    static UdsResponse defaultResponse(const ByteArray &request) {
        (void)request;
        return {UdsResponseCode::ServiceNotSupported, {}};
    }

    static ByteArray makeResponse(const ByteArray &request, UdsResponseCode responseCode = UdsResponseCode::OK, const ByteArray &extraData = {}) {
        if (responseCode != UdsResponseCode::OK) {
            ByteArray negativeResponse;
            negativeResponse.emplace_back(0x7F);                                // Negative response indicator
            negativeResponse.emplace_back(request.empty() ? 0x00 : request[0]); // Original service ID or 0
            negativeResponse.emplace_back(static_cast<uint8_t>(responseCode));  // NRC
            return negativeResponse;
        }

        ByteArray positiveResponse;
        positiveResponse.emplace_back(static_cast<uint8_t>((request.empty() ? 0x00 : request[0]) + UDS_POSITIVE_RESPONSE_OFFSET)); // Positive response SID
        positiveResponse.insert(positiveResponse.end(), extraData.begin(), extraData.end());
        return positiveResponse;
    }

    std::unordered_map<uint8_t, IUdsServiceHandlerPtr> m_handlers;
};

} // namespace doip::uds

#endif /* UDSMOCK_H */

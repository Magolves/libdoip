#include "uds/UdsMock.h"
#include "DoIPMessage.h"

namespace doip::uds {

void UdsMock::registerDiagnosticSessionControlHandler(std::function<UdsResponse(uint8_t)> handler) {
    registerService(UdsService::DiagnosticSessionControl, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint8_t sessionType = req[1];
        return handler(sessionType);
    });
}

void UdsMock::registerECUResetHandler(std::function<UdsResponse(uint8_t)> handler) {
    registerService(UdsService::ECUReset, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint8_t resetType = req[1];
        return handler(resetType);
    });
}

void UdsMock::registerReadDataByIdentifierHandler(std::function<UdsResponse(uint16_t)> handler) {
    registerService(UdsService::ReadDataByIdentifier, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint16_t did = (static_cast<uint16_t>(req[1]) << 8) | req[2];
        return handler(did);
    });
}

void UdsMock::registerWriteDataByIdentifierHandler(std::function<UdsResponse(uint16_t, ByteArray)> handler) {
    registerService(UdsService::WriteDataByIdentifier, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint16_t did = (static_cast<uint16_t>(req[1]) << 8) | req[2];
        return handler(did, ByteArray(req.data() + 3, req.size() - 3));
    });
}


void UdsMock::registerTesterPresentHandler(std::function<UdsResponse(uint8_t)> handler) {
    registerService(UdsService::TesterPresent, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint8_t subFunction = req[1];
        return handler(subFunction);
    });
}

} // namespace doip::uds

#include "uds/UdsMock.h"
#include "DoIPMessage.h"

namespace doip::uds {

ByteArray UdsMock::handleDiagnosticRequest(const ByteArray &request) const {
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
                  << " out of bounds for service 0x" << std::hex << static_cast<int>(service) << std::dec
                  << " (expected " << desc->minReqLength << "-" << desc->maxReqLength << ")\n";
        return makeResponse(request, UdsResponseCode::IncorrectMessageLengthOrInvalidFormat);
    }

    UdsResponse resp = {UdsResponseCode::ServiceNotSupported, {}};
    auto it = m_handlers.find(sid);
    if (it != m_handlers.end() && it->second) {
        resp = it->second->handle(request);
    } else {
        return makeResponse(request, UdsResponseCode::ServiceNotSupported);
    }

    auto rspSize = resp.second.size() + 1; // +1 for the SID
    if (rspSize < desc->minRspLength || rspSize > desc->maxRspLength) {
        std::cerr << "UdsMock: Response length " << resp.second.size()
                  << " out of bounds for service 0x" << std::hex << static_cast<int>(service) << std::dec
                  << " (expected " << desc->minRspLength << "-" << desc->maxRspLength << ")\n";
        return makeResponse(request, UdsResponseCode::GeneralProgrammingFailure, {});
    }

    return makeResponse(request, resp.first, resp.second);
}

void UdsMock::registerDiagnosticSessionControlHandler(std::function<UdsResponse(uint8_t)> handler) {
    registerService(UdsService::DiagnosticSessionControl, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint8_t sessionType = req[1];
        return handler(sessionType);
    });
}

void UdsMock::registerEcuResetHandler(std::function<UdsResponse(uint8_t)> handler) {
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

// Start download (0x34): handler(memoryAddress, length)
void UdsMock::registerRequestDownloadHandler(std::function<UdsResponse(uint32_t memoryAddress, uint32_t length)> handler)
{
    registerService(UdsService::RequestDownload, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        // For simplicity, assume memory address and length are 4 bytes each, big-endian
        if (req.size() < 10) { // SID(1) + Addr(4) + Length(4) + at least 1 byte of data
            return {UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, {}};
        }
        uint32_t memoryAddress = (static_cast<uint32_t>(req[1]) << 24) |
                                 (static_cast<uint32_t>(req[2]) << 16) |
                                 (static_cast<uint32_t>(req[3]) << 8) |
                                 req[4];
        uint32_t length = (static_cast<uint32_t>(req[5]) << 24) |
                          (static_cast<uint32_t>(req[6]) << 16) |
                          (static_cast<uint32_t>(req[7]) << 8) |
                          req[8];
        return handler(memoryAddress, length);
    });
}

// Transfer Data (0x36): handler(blockSequenceCounter, data)
void UdsMock::registerTransferDataHandler(std::function<UdsResponse(uint8_t blockSequenceCounter, const ByteArray &data)> handler) {
    registerService(UdsService::TransferData, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        uint8_t blockSequenceCounter = req[1];
        return handler(blockSequenceCounter, ByteArray(req.data() + 2, req.size() - 2));
    });
}

//End dowlnload (0x37): handler()
void UdsMock::registerRequestTransferExitHandler(std::function<UdsResponse()> handler) {
    registerService(UdsService::RequestTransferExit, [handler = std::move(handler)](const ByteArray &req) -> UdsResponse {
        (void)req;
        return handler();
    });
}

} // namespace doip::uds

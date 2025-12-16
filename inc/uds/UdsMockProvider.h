#pragma once
#include "ByteArray.h"
#include "IDownstreamProvider.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMock.h"
#include <atomic>
#include <thread>

namespace doip::uds {

using namespace std::chrono_literals;
using namespace doip;

class UdsMockProvider : public IDownstreamProvider {
  public:
    UdsMockProvider() {
        m_uds.registerDefaultServices();
        m_uds.registerDiagnosticSessionControlHandler(
            [this](uint8_t sessionType) -> UdsResponse {
                ByteArray rsp;
                rsp.push_back(sessionType);
                rsp.writeU16BE(m_p2_ms);
                rsp.writeU16BE(m_p2star_10ms);
                return UdsResponse(UdsResponseCode::PositiveResponse, rsp);
            });
        m_uds.registerReadDataByIdentifierHandler(
            [](uint16_t did) -> UdsResponse {
                ByteArray rsp;
                if (did == 0xF190) {     // Vehicle Identification Number (VIN)
                    rsp.push_back(0xF1);
                    rsp.push_back(0x90);
                    // Example VIN: "1HGCM82633A123456"
                    const std::string vin = "1HGCM82633A123456";
                    rsp.insert(rsp.end(), vin.begin(), vin.end());
                    return UdsResponse(UdsResponseCode::PositiveResponse, rsp);
                } else {
                    return UdsResponse(UdsResponseCode::RequestOutOfRange, {});
                }
            });
        m_uds.registerTesterPresentHandler(
            [](uint8_t subFunction) -> UdsResponse {
                ByteArray rsp;
                rsp.push_back(subFunction);
                return UdsResponse(UdsResponseCode::PositiveResponse, rsp);
            });

        m_uds.registerEcuResetHandler(
            [](uint8_t resetType) -> UdsResponse {
                ByteArray rsp;
                rsp.push_back(resetType);
                return UdsResponse(UdsResponseCode::PositiveResponse, rsp);
            });

        m_uds.registerWriteDataByIdentifierHandler(
            [](uint16_t did, ByteArray value) -> UdsResponse {
                (void)value; // Ignore value for mock
                ByteArray rsp;
                rsp.push_back(static_cast<uint8_t>((did >> 8) & 0xFF));
                rsp.push_back(static_cast<uint8_t>(did & 0xFF));
                return UdsResponse(UdsResponseCode::PositiveResponse, rsp);
            });
    }

    ~UdsMockProvider() override = default;

    // void start() override {
    // }

    // void stop() override {
    // }

    void sendRequest(const ByteArray request, DownstreamCallback cb) override {
        if (!cb)
            return;

        auto start_ts = std::chrono::steady_clock::now();

        // Synchronous UDS processing
        ByteArray rsp = m_uds.handleDiagnosticRequest(request);

        auto end_ts = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts);

        DownstreamResponse dr;
        dr.payload = rsp;
        dr.latency = latency;
        dr.status = DownstreamStatus::Handled;
        cb(dr); // Invoke the callback immediately
    }

  private:
    uds::UdsMock m_uds;
    uint16_t m_p2_ms = 1000;
    uint16_t m_p2star_10ms = 200;
};

} // namespace doip::uds

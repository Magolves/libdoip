/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef EXAMPLEDOIPSERVERMODEL_H
#define EXAMPLEDOIPSERVERMODEL_H

#include "DoIPServerModel.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMock.h"
#include "uds/UdsResponseCode.h"

using namespace doip;

class ExampleDoIPServerModel : public DoIPServerModel {
  public:
    ExampleDoIPServerModel() {
        onOpenConnection = [this](IConnectionContext &ctx) noexcept {
            (void)ctx;
            startWorker();
        };
        onCloseConnection = [this](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
            (void)ctx;
            stopWorker();
            LOG_DOIP_WARN("Connection closed ({})", fmt::streamed(reason));
        };

        onDiagnosticMessage = [this](IConnectionContext &ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            m_log->info("Received Diagnostic message (from ExampleDoIPServerModel)", fmt::streamed(msg));

            // Example: Access payload using getPayload()
            // auto payload = msg.getDiagnosticMessagePayload();
            // if (payload.second >= 3 && payload.first[0] == 0x22 && payload.first[1] == 0xF1 && payload.first[2] == 0x90) {
            //     m_log->info(" - Detected Read Data by Identifier for VIN (0xF190) -> send NACK");
            //     return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
            // }

            return std::nullopt;
        };

        onDiagnosticNotification = [this](IConnectionContext &ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            m_log->info("Diagnostic ACK/NACK sent (from ExampleDoIPServerModel)", fmt::streamed(ack));
        };

        onDownstreamRequest = [this](IConnectionContext &ctx, const DoIPMessage &msg, ServerModelDownstreamResponseHandler callback) noexcept {
            (void)ctx;
            (void)msg;

            m_log->info("Received downstream request (from ExampleDoIPServerModel)", fmt::streamed(msg));
            m_downstreamCallback = callback;
            if (!m_downstreamCallback) {
                m_log->error("onDownstreamRequest: No callback function passed");
                return DoIPDownstreamResult::Error;
            }

            // Store message in send queue
            auto [data, size] = msg.getDiagnosticMessagePayload();
            m_tx.push(ByteArray(data, size));
            m_log->info("Enqueued msg");
            return DoIPDownstreamResult::Pending;
        };

        m_uds.registerDefaultServices();

        m_uds.registerDiagnosticSessionControlHandler([this](uint8_t sessionType) {
            m_loguds->info("Diagnostic Session Control requested, sessionType={:02X}", sessionType);
            auto response = ByteArray{sessionType}; // Positive response SID = 0x50
            response.writeU16BE(m_p2_ms);
            response.writeU16BE(m_p2star_10ms);
            return std::make_pair(uds::UdsResponseCode::PositiveResponse, response); // Positive response
        });

        m_uds.registerECUResetHandler([this](uint8_t resetType) {
            m_loguds->info("ECU Reset requested, resetType={:02X}", resetType);
            return std::make_pair(uds::UdsResponseCode::PositiveResponse, ByteArray{resetType}); // Positive response SID = 0x61
        });

        m_uds.registerReadDataByIdentifierHandler([this](uint16_t did) {
            m_loguds->info("Read Data By Identifier requested, DID={:04X}", did);
            if (did == 0xF190) {
                // Return example VIN
                ByteArray vinPayload = {'1', 'H', 'G', 'C', 'M',
                                        '8', '2', '6', '3', '3',
                                        'A', '0', '0', '0', '0', '1', 'Z'};
                ByteArray response = {static_cast<uint8_t>((did >> 8) & 0xFF), static_cast<uint8_t>(did & 0xFF)};
                response.insert(response.end(), vinPayload.begin(), vinPayload.end());
                return std::make_pair(uds::UdsResponseCode::PositiveResponse, response); // Positive response
            }
            return std::make_pair(uds::UdsResponseCode::RequestOutOfRange, ByteArray{0x22}); // Positive response
        });

        m_uds.registerWriteDataByIdentifierHandler([this](uint16_t did, ByteArray value) {
            m_loguds->info("Write Data By Identifier requested, DID={:04X}, value={}", did, fmt::streamed(value));
            if (did == 0xF190) {
                // Accept VIN write
                return std::make_pair(uds::UdsResponseCode::PositiveResponse, ByteArray{static_cast<uint8_t>((did >> 8) & 0xFF), static_cast<uint8_t>(did & 0xFF)}); // Positive response
            }
            return std::make_pair(uds::UdsResponseCode::RequestOutOfRange, ByteArray{0x2E}); // NRC for WriteDataByIdentifier
        });

        m_uds.registerTesterPresentHandler([this](uint8_t subFunction) {
            m_loguds->info("Tester Present requested, subFunction={:02X}", subFunction);
            return std::make_pair(uds::UdsResponseCode::PositiveResponse, ByteArray{0x00}); // Positive response SID = 0x7E
        });
    }

  private:
    std::shared_ptr<spdlog::logger> m_log = Logger::get("smodel");
    std::shared_ptr<spdlog::logger> m_loguds = Logger::get("uds");
    ServerModelDownstreamResponseHandler m_downstreamCallback = nullptr;
    ThreadSafeQueue<ByteArray> m_rx;
    ThreadSafeQueue<ByteArray> m_tx;
    uds::UdsMock m_uds;
    std::thread m_worker;
    bool m_running = true;
    uint16_t m_p2_ms = 1000;
    uint16_t m_p2star_10ms = 200;


    void startWorker() {
        m_worker = std::thread([this] {
            while (m_running) {
                downstream_thread();
            }
        });
        m_log->info("Started worker thread");
    }
    void stopWorker() {
        m_running = false;
        if (m_worker.joinable())
            m_worker.join();
        m_log->info("Stopped worker thread");
    }

    /**
     * @brief Thread simulating downstream communication (e. g. CAN).
     */
    void downstream_thread() {
        if (m_tx.size()) {
            // simulate send. In a real environment we could send a CAN message
            ByteArray req;
            m_tx.pop(req);
            m_log->info("Simulate send {}", fmt::streamed(req));
            // simulate some latency
            std::this_thread::sleep_for(50ms);
            // simulate receive
            auto msg = m_uds.handleDiagnosticRequest(req);
            m_log->info("Simulate UDS response {}", fmt::streamed(msg));
            m_rx.push(msg);
        }

        if (m_rx.size()) {
            ByteArray rsp;
            m_rx.pop(rsp);
            m_log->info("Simulate receive {}", fmt::streamed(rsp));
            if (m_downstreamCallback) {
                m_downstreamCallback(rsp, DoIPDownstreamResult::Handled);
                m_downstreamCallback = nullptr;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */

/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#include "DoIPServerModel.h"
#include "ThreadSafeQueue.h"
#include "IDownstreamProvider.h"

using namespace doip;
using namespace std::chrono_literals;

class DoIPDownstreamServerModel : public DoIPServerModel {
  public:
    DoIPDownstreamServerModel(const std::string& name, IDownstreamProvider& provider) : m_log(Logger::get(name)), m_provider(provider) {


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
            m_log->info("Received Diagnostic message (from DoIPDownstreamServerModel)", fmt::streamed(msg));

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
            m_log->info("Diagnostic ACK/NACK sent (from DoIPDownstreamServerModel)", fmt::streamed(ack));
        };

        onDownstreamRequest = [this](IConnectionContext &ctx, const DoIPMessage &msg, ServerModelDownstreamResponseHandler callback) noexcept {
            (void)ctx;
            (void)msg;

            m_log->info("Received downstream request (from DoIPDownstreamServerModel)", fmt::streamed(msg));
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

    }
    protected:
    virtual void handleDownstreamResponse(const DownstreamResponse &response) {
        m_log->info("Handle downstream response {} [latency {}ms]", fmt::streamed(response.payload), response.latency.count());
        m_rx.push(response.payload);
    }

  private:
    std::shared_ptr<spdlog::logger> m_log ;

    ServerModelDownstreamResponseHandler m_downstreamCallback = nullptr;
    ThreadSafeQueue<ByteArray> m_rx;
    ThreadSafeQueue<ByteArray> m_tx;
    IDownstreamProvider& m_provider;

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
            m_log->info("Send {}", fmt::streamed(req));
            // simulate some latency
            std::this_thread::sleep_for(50ms);
            // simulate receive
            m_provider.sendRequest(req, [this](const DownstreamResponse &resp) {
                handleDownstreamResponse(resp);
            } );

        }

        if (m_rx.size()) {
            ByteArray rsp;
            m_rx.pop(rsp);
            m_log->info("Receive {}", fmt::streamed(rsp));
            if (m_downstreamCallback) {
                m_downstreamCallback(rsp, DoIPDownstreamResult::Handled);
                m_downstreamCallback = nullptr;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
};

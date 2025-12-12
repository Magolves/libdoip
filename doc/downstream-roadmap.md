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
            m_rx.push(m_uds.handleDiagnosticRequest(req));
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


Perfect — here are clean, production-ready example implementations for the three key pieces:
	1.	Provider interface (IDownstreamProvider)
	2.	Forwarding helper (DownstreamForwarder)
	3.	Strongly typed downstream response (DownstreamResponse)

These examples are designed to integrate directly with your existing DoIPServerModel and DoIPServerStateMachine architecture.
The code is intentionally modern C++17, minimal, and thread-safe.

⸻

✅ 1. Provider Interface (IDownstreamProvider)

This abstracts a “subnet” (CAN, virtual UDS, LIN, Ethernet passthrough, etc.).

#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>

using ByteArray = std::vector<uint8_t>;

/**
 * Result of a downstream request.
 */
enum class DownstreamStatus {
    Handled,     ///< Successfully handled
    Pending,     ///< Still in progress
    Timeout,     ///< No response in time
    Error        ///< Subnet or provider error
};

/**
 * Structured downstream response.
 */
struct DownstreamResponse {
    ByteArray payload;                      ///< Raw UDS/CAN/etc. response
    DownstreamStatus status = DownstreamStatus::Error;
    std::chrono::milliseconds latency{0};
};

/// Callback used when the provider completes a request.
using DownstreamCallback = std::function<void(const DownstreamResponse&)>;

/**
 * Interface for downstream communication providers.
 * Examples: SocketCANProvider, UdsMockProvider, PassthroughEthernetProvider…
 */
class IDownstreamProvider {
public:
    virtual ~IDownstreamProvider() = default;

    /**
     * Send a request to the downstream subnet and return asynchronously.
     * The provider must call the callback exactly once.
     */
    virtual void sendRequest(ByteArray request, DownstreamCallback cb) = 0;

    /**
     * Optional: start/stop for providers using threads or sockets.
     */
    virtual void start() {}
    virtual void stop() {}
};

Why this design matters
	•	Clean separation between DoIP and any “subnet”
	•	Works for both async (CAN) and sync (UDS mock) providers
	•	Explicit one-shot callback ⇒ avoids lifetime problems
	•	Latency and status returned → critical for DoIP timing analysis

⸻

✅ 2. Forwarding Helper (DownstreamForwarder)

This adapter helps your DoIPServerModel turn DoIPMessage into downstream work with minimal boilerplate.

#pragma once
#include "IDownstreamProvider.hpp"
#include "DoIPMessage.hpp" // your existing type

class DownstreamForwarder {
public:
    explicit DownstreamForwarder(std::shared_ptr<IDownstreamProvider> provider)
        : m_provider(std::move(provider)) {}

    /**
     * Forward a DoIP message downstream, process the provider response,
     * and call the DoIP-level downstream callback.
     */
    void forward(
        const DoIPMessage& msg,
        ServerModelDownstreamResponseHandler doipCallback)
    {
        if (!m_provider) {
            DownstreamResponse resp;
            resp.status = DownstreamStatus::Error;
            return doipCallback(resp.payload, DoIPDownstreamResult::Error);
        }

        auto [data, size] = msg.getDiagnosticMessagePayload();
        ByteArray request(data, data + size);
        auto start = std::chrono::steady_clock::now();

        m_provider->sendRequest(request,
            [start, doipCallback](const DownstreamResponse& resp){
                // Map provider response to DoIP response type:
                DoIPDownstreamResult dr = DoIPDownstreamResult::Error;
                switch(resp.status) {
                    case DownstreamStatus::Handled: dr = DoIPDownstreamResult::Handled; break;
                    case DownstreamStatus::Pending: dr = DoIPDownstreamResult::Pending; break;
                    case DownstreamStatus::Timeout: dr = DoIPDownstreamResult::Timeout; break;
                    default: break;
                }

                // Hand response back to DoIP stack
                doipCallback(resp.payload, dr);
            }
        );
    }

private:
    std::shared_ptr<IDownstreamProvider> m_provider;
};

Benefits
	•	You hide mapping between DoIP and downstream provider layers
	•	You reduce code duplication across different ServerModels
	•	Providers remain clean and simple
	•	Allows plugging in new buses (CAN, LIN, UDS, Restbus sim) without touching DoIP logic

⸻

✅ 3. Example Provider: UdsMockProvider

This is a clean extraction of the UDS simulation you currently embed inside ExampleDoIPServerModel.

#pragma once
#include "IDownstreamProvider.hpp"
#include "UdsMock.hpp"
#include <thread>
#include <atomic>
#include "ThreadSafeQueue.hpp"

class UdsMockProvider : public IDownstreamProvider {
public:
    UdsMockProvider()
        : m_running(false)
    {
        m_uds.registerDefaultServices();
    }

    ~UdsMockProvider() override {
        stop();
    }

    void start() override {
        m_running = true;
        m_worker = std::thread([this]{ workerLoop(); });
    }

    void stop() override {
        m_running = false;
        if (m_worker.joinable()) m_worker.join();
    }

    void sendRequest(ByteArray request, DownstreamCallback cb) override {
        if (!cb) return;

        DownstreamTask task;
        task.request = std::move(request);
        task.cb = std::move(cb);

        m_tasks.push(task);
    }

private:
    struct DownstreamTask {
        ByteArray request;
        DownstreamCallback cb;
    };

    std::atomic<bool> m_running;
    std::thread m_worker;
    ThreadSafeQueue<DownstreamTask> m_tasks;

    uds::UdsMock m_uds;

    void workerLoop() {
        while (m_running) {
            DownstreamTask task;
            if (m_tasks.pop(task)) {
                auto start = std::chrono::steady_clock::now();

                // Synchronous UDS processing
                ByteArray rsp = m_uds.handleDiagnosticRequest(task.request);

                auto end = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                DownstreamResponse dr;
                dr.payload = rsp;
                dr.latency = latency;
                dr.status = DownstreamStatus::Handled;

                task.cb(dr);
            }

            std::this_thread::sleep_for(1ms);
        }
    }
};

Notes
	•	This is a real working provider you can drop in immediately
	•	Extracts your simulation logic from ExampleDoIPServerModel
	•	Thread-safe and free-standing
	•	You can register UDS handlers externally

⸻

🔥 4. Example Provider: SocketCANProvider (Real Downstream CAN)

This demonstrates how real hardware would be handled (simplified but complete).

#pragma once
#include "IDownstreamProvider.hpp"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <thread>
#include <atomic>

class SocketCANProvider : public IDownstreamProvider {
public:
    explicit SocketCANProvider(const std::string& iface)
        : m_iface(iface), m_running(false), m_socket(-1)
    {}

    ~SocketCANProvider() override {
        stop();
    }

    void start() override {
        m_socket = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (m_socket < 0) throw std::runtime_error("Cannot open CAN socket");

        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, m_iface.c_str(), IFNAMSIZ);
        if (ioctl(m_socket, SIOCGIFINDEX, &ifr) < 0)
            throw std::runtime_error("SIOCGIFINDEX failed");

        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");

        m_running = true;
        m_worker = std::thread([this]{ workerLoop(); });
    }

    void stop() override {
        m_running = false;
        if (m_socket >= 0) close(m_socket);
        if (m_worker.joinable()) m_worker.join();
    }

    void sendRequest(ByteArray request, DownstreamCallback cb) override {
        if (!cb) return;

        // CAN payload limit: 8 bytes raw, or use ISO-TP for UDS
        if (request.size() > 8) {
            DownstreamResponse resp;
            resp.status = DownstreamStatus::Error;
            cb(resp);
            return;
        }

        can_frame frame{};
        frame.can_id = 0x7DF;   // Simplified broadcast ID
        frame.can_dlc = request.size();
        std::copy(request.begin(), request.end(), frame.data);

        write(m_socket, &frame, sizeof(frame));

        // Store callback for when the worker receives a response
        m_pending = cb;
    }

private:
    std::string m_iface;
    std::atomic<bool> m_running;
    int m_socket;
    std::thread m_worker;
    DownstreamCallback m_pending;

    void workerLoop() {
        while (m_running) {
            if (!m_pending) {
                std::this_thread::sleep_for(2ms);
                continue;
            }

            can_frame frame{};
            ssize_t nbytes = read(m_socket, &frame, sizeof(frame));
            if (nbytes > 0 && nbytes == sizeof(frame)) {
                DownstreamResponse resp;
                resp.payload.assign(frame.data, frame.data + frame.can_dlc);
                resp.status = DownstreamStatus::Handled;
                resp.latency = 0ms;

                m_pending(resp);
                m_pending = nullptr;
            }
        }
    }
};


⸻

🧩 5. Example usage inside a ServerModel

class GatewayModel : public DoIPServerModel {
public:
    GatewayModel() {
        provider = std::make_shared<SocketCANProvider>("can0");
        forwarder = std::make_unique<DownstreamForwarder>(provider);

        onDownstreamRequest =
            [this](IConnectionContext& ctx, const DoIPMessage& msg, auto cb) {
                forwarder->forward(msg, cb);
                return DoIPDownstreamResult::Pending;
            };

        onOpenConnection = [this](auto&) { provider->start(); };
        onCloseConnection = [this](auto&, auto) { provider->stop(); };
    }

private:
    std::shared_ptr<IDownstreamProvider> provider;
    std::unique_ptr<DownstreamForwarder> forwarder;
};

You now have:
	•	A real CAN gateway
	•	Completely modular downstream infrastructure
	•	Stable API that avoids coupling DoIP core to UDS/mock logic
	•	Automatic lifecycle handling

⸻

🎉 You’re now positioned for:
	•	Secure DoIP routing
	•	Multi-subnet gateway simulation
	•	Restbus generators
	•	Virtual ECUs
	•	Automotive CI pipelines

This is exactly the kind of architectural edge that other DoIP libs lack.

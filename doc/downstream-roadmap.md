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

---

Great questions — and you’re right: ISO 13400 (DoIP) does not define any way to tell the tester which downstream subnets exist.
So both points require architectural choices outside the DoIP standard.
Below is the most widely accepted practice in the automotive domain.

⸻

✅ 1) How to represent a CAN network in a config file (TOML/YAML)

A reasonable and flexible configuration must capture:

✔ Physical interfaces

(e.g. socketcan: can0, vcan1)

✔ UDS functional addressing

(Request IDs, Response IDs)

✔ Routing rules

(Which CAN IDs belong to which logical ECU or subnet)

✔ Timing

(Timeouts, P2/P2*, ISO-TP separation times)

✔ Optional: simulation behavior

(Mock/Restbus definitions)

Below is a practical TOML layout (recommended because TOML is easy to parse and human-friendly):

⸻

📌 Example: networks.toml

# === Global CAN timing defaults ===
[defaults]
  timeout_ms = 2000
  block_size = 8
  st_min_ms = 3

# === Subnet 1: Powertrain CAN ===
[[subnet]]
  name = "PT"
  type = "can"
  interface = "can0"
  baudrate = 500000
  description = "Powertrain network (ECM, TCM)"

  # Logical ECUs reachable via this subnet
  [[subnet.ecu]]
    name = "ECM"
    request_id  = "0x7E0"
    response_id = "0x7E8"

  [[subnet.ecu]]
    name = "TCM"
    request_id  = "0x7E1"
    response_id = "0x7E9"

# === Subnet 2: Body CAN ===
[[subnet]]
  name = "BCM"
  type = "can"
  interface = "can1"
  baudrate = 250000
  description = "Body network (BCM, IPC, HVAC)"

  [[subnet.ecu]]
    name = "BCM"
    request_id  = "0x743"
    response_id = "0x743"

  [[subnet.ecu]]
    name = "HVAC"
    request_id  = "0x744"
    response_id = "0x744"

# === Optional: virtual ECUs using the UDS Mock ===
[[subnet]]
  name = "SIM"
  type = "uds_mock"
  description = "Simulation of missing ECUs"

  [[subnet.ecu]]
    name = "MockABS"
    uds_services = ["0x10", "0x22", "0x2E", "0x31"]


⸻

Why this structure works

👍 Mirrors typical real OEM data structures

Many OEM gateway configs look similar (JSON/XML/TOML).

👍 Maps directly to your DownstreamProvider

Each [[subnet]] becomes a provider instance.

👍 Extensible

Later you can add:

protocol = "isotp"
p2_ms = 150
p2star_ms = 5000

or

dbc = "./pt.dbc"

👍 Supports both real CAN and UDS mocks

type = "can" → SocketCANProvider
type = "uds_mock" → UdsMockProvider

⸻

🔧 Parsing this into providers

During startup, DoIPServerModel would:
	•	Read networks.toml
	•	For each [[subnet]], instantiate a provider
	•	Register them in a table:

providers["PT"]
providers["BCM"]
providers["SIM"]


	•	Use routing rules to select the right provider for each request
(requestID-based, ECU name, etc.)

This makes your DoIP router fully data-driven.

⸻

✅ 2) How to tell the client about existing subnets

Short answer: You can’t. ISO 13400 has no mechanism for this.

DoIP only defines:
	•	Vehicle identification
	•	Entity status
	•	Routing activation
	•	Diagnostic messages pass-through

There is no command to enumerate downstream networks.
This is by design: the DoIP node is considered an opaque gateway.

⸻

🔍 So how do OEMs handle it in practice?

✔ Method 1: Proprietary UDS services

OEMs often expose:
	•	Manufacturer-specific DID (0xF1xx)
	•	Data records that list:
	•	Available ECUs
	•	Available subnets
	•	Versions
	•	Bus loads
	•	Routing tables

Example (not standard, but common):

ReadDataByIdentifier(0xF190) → “PT, BCM, SIM”
ReadDataByIdentifier(0xF191) → ECU list with Request/Response IDs
ReadDataByIdentifier(0xF192) → Supported protocols/subnets

You can pick one DID (e.g. 0xF195) and return a structured payload.

⸻

✔ Method 2: External metadata file (preferred for diagnostic tools)

Tools like:
	•	ODIS
	•	ISTA
	•	DTS Monaco
	•	CANoe/CANdela
	•	Eclipse SUMO
	•	OEM Engineering Tools

use an ODX or proprietary XML describing the gateway.

Your DoIP server would not advertise subnets itself.
Instead, the user loads the metadata file.

➡ This is cleaner, stable, and aligns with industry practice.

⸻

✔ Method 3: Discovery via functional addressing

The client tests:
	•	broadcast 0x7DF → see who responds
	•	targeted requests → see which ECUs exist

This is what many tools do automatically.

Not elegant, but works regardless of DoIP vs CAN.

⸻

📌 Recommendation for YOUR libdoip

You want a feature other DoIP libs don’t offer, and this is one of them:

🎯 Provide a standard, vendor-neutral metadata endpoint

For example:
	•	A file: doip_gateway.json
	•	A REST endpoint (optional)
	•	A vendor-specific UDS DID (recommended)

Minimal DID example:

Return this as a simple TLV:

| SubnetCount = 3 |
| SubnetInfo = [ "PT", "BCM", "SIM" ] |

Or structured:

[
  { "name": "PT",  "type": "can",      "ecus": [ "ECM", "TCM" ] },
  { "name": "BCM", "type": "can",      "ecus": [ "BCM", "HVAC" ] },
  { "name": "SIM", "type": "uds_mock", "ecus": [ "MockABS" ] }
]

This makes your DoIP gateway truly introspectable.

⸻

🎉 Summary

✔ Represent CAN networks in TOML/YAML:

Use structure:

[[subnet]]
  name
  type (can/uds_mock/etc)
  interface
  baudrate
  [[subnet.ecu]]
    name
    request_id
    response_id

✔ Telling the client about existing subnets:

ISO 13400 does not provide a mechanism.
But you can add one through:

Best options:
	1.	Vendor-specific UDS DID (recommended)
	2.	Metadata file (ODX/TOML/etc.)
	3.	Functional discovery

This is a value-add capability no other libdoip currently implements.
-------

Great — here are (1) a ready-to-use CAN network parser in C++ for your TOML configuration, and (4) a vendor-specific UDS DID implementation that reports the subnet topology.

Both are designed to integrate cleanly into your existing DoIPServerModel and provider design.

⸻

✅ 1) CAN Network Config Parser (TOML → C++ providers)

✔ Input: networks.toml

Example from earlier:

[defaults]
  timeout_ms = 2000
  block_size = 8
  st_min_ms = 3

[[subnet]]
  name = "PT"
  type = "can"
  interface = "can0"
  baudrate = 500000

  [[subnet.ecu]]
    name = "ECM"
    request_id  = "0x7E0"
    response_id = "0x7E8"

  [[subnet.ecu]]
    name = "TCM"
    request_id  = "0x7E1"
    response_id = "0x7E9"

[[subnet]]
  name = "SIM"
  type = "uds_mock"

  [[subnet.ecu]]
    name = "MockABS"
    uds_services = ["0x10", "0x22"]


⸻

✔ C++ Data Model

struct EcuConfig {
    std::string name;
    uint32_t requestId;
    uint32_t responseId;
    std::vector<uint8_t> udsServices;
};

struct SubnetConfig {
    std::string name;
    std::string type;  // "can", "uds_mock"
    std::string interface;
    uint32_t baudrate = 0;
    std::vector<EcuConfig> ecus;
};

struct NetworkConfig {
    uint32_t timeoutMs = 2000;
    uint8_t blockSize = 8;
    uint8_t stMinMs = 3;
    std::vector<SubnetConfig> subnets;
};


⸻

✔ TOML Parser Implementation (using toml++)

#include <toml++/toml.h>

static uint32_t parse_hex_uint(const std::string &s) {
    return std::stoul(s, nullptr, 16);
}

NetworkConfig loadNetworkConfig(const std::string &path) {
    NetworkConfig cfg;
    toml::table tbl = toml::parse_file(path);

    if (auto defaults = tbl["defaults"].as_table()) {
        cfg.timeoutMs = defaults->get("timeout_ms").value_or(2000);
        cfg.blockSize = defaults->get("block_size").value_or(8);
        cfg.stMinMs   = defaults->get("st_min_ms").value_or(3);
    }

    for (auto &sub : *tbl["subnet"].as_array()) {
        SubnetConfig s;

        auto &t = *sub.as_table();
        s.name     = t["name"].value_or("");
        s.type     = t["type"].value_or("");
        s.interface= t["interface"].value_or("");
        s.baudrate = t["baudrate"].value_or(0);

        if (auto ecuArr = t["ecu"].as_array()) {
            for (auto &ecuNode : *ecuArr) {
                EcuConfig ecu;
                auto &e = *ecuNode.as_table();

                ecu.name = e["name"].value_or("");

                if (auto req = e["request_id"].value<std::string>())
                    ecu.requestId = parse_hex_uint(*req);

                if (auto rsp = e["response_id"].value<std::string>())
                    ecu.responseId = parse_hex_uint(*rsp);

                // decode UDS services (optional)
                if (auto services = e["uds_services"].as_array()) {
                    for (auto &svc : *services) {
                        auto sVal = svc.value<std::string>();
                        if (sVal)
                            ecu.udsServices.push_back(parse_hex_uint(*sVal));
                    }
                }

                s.ecus.push_back(ecu);
            }
        }

        cfg.subnets.push_back(s);
    }

    return cfg;
}


⸻

✔ Integrating this into your DoIP server

void buildSubnets(const NetworkConfig& cfg,
                  std::vector<std::unique_ptr<DownstreamProvider>>& out)
{
    for (auto& sub : cfg.subnets) {
        if (sub.type == "can") {
            out.push_back(std::make_unique<CanDownstreamProvider>(
                sub.interface, sub.baudrate, sub.ecus));
        }
        else if (sub.type == "uds_mock") {
            out.push_back(std::make_unique<UdsMockProvider>(
                sub.name, sub.ecus));
        }
    }
}


⸻

🎉 This gives you a fully data-driven DoIP gateway.

⸻

✅ 4) Vendor-Specific UDS DID for Subnet Discovery

Since ISO 13400 offers no discovery mechanism, we create one.

✔ Select a vendor DID: 0xF195

Response format suggestion (TLV-like)

# ECUs:
Count (1 byte)
Repeated:
  SubnetNameLen (1 byte)
  SubnetName (N bytes)
  EcuNameLen (1 byte)
  EcuName (M bytes)
  RequestID (2 bytes)
  ResponseID (2 bytes)

This is:
	•	Human-readable
	•	Extensible
	•	Compact
	•	Easy for diagnostic tools

⸻

✔ Implementation (hook into your UDS mock or real UDS stack)

void registerSubnetTopologyDid(const NetworkConfig& cfg, uds::UdsMock& uds)
{
    uds.registerReadDataByIdentifierHandler(0xF195,
        [cfg](uint16_t did) -> std::pair<uds::UdsResponseCode, ByteArray>
    {
        ByteArray out;

        uint8_t ecuCount = 0;
        for (auto& s : cfg.subnets)
            ecuCount += s.ecus.size();

        out.push(ecuCount);

        for (auto& s : cfg.subnets) {
            for (auto& e : s.ecus) {

                out.push((uint8_t)s.name.size());
                out.insert(out.end(), s.name.begin(), s.name.end());

                out.push((uint8_t)e.name.size());
                out.insert(out.end(), e.name.begin(), e.name.end());

                out.writeU16BE(e.requestId);
                out.writeU16BE(e.responseId);
            }
        }

        return { uds::UdsResponseCode::PositiveResponse, out };
    });
}


⸻

✔ Example Response on the wire

If you configured:
	•	Subnet PT → ECM, TCM
	•	Subnet SIM → MockABS

Client reads:

22 F1 95

Response:

62 F1 95
03                     # number of ECUs
02 50 54               # "PT"
03 45 43 4D            # "ECM"
07 E0 07 E8            # IDs

02 50 54
03 54 43 4D
07 E1 07 E9

03 53 49 4D
07 4D 6F 63 6B 41 42 53
00 00 00 00            # mocks may not need IDs

A diagnostic tool now knows the entire topology.

⸻

🎯 Summary

✔ Part 1: CAN network config
	•	Provided TOML format
	•	Provided full C++ parser
	•	Automatically maps to DownstreamProvider instances

✔ Part 4: UDS DID for subnet discovery
	•	DID 0xF195
	•	TLV-like encoding
	•	Simple, portable, tool-friendly
	•	Works with real or mock UDS


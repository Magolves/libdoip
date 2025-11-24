#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "DoIPServer.h"


using namespace doip;
using namespace std;

static const DoIPAddress LOGICAL_ADDRESS(static_cast<uint8_t>(0x0), static_cast<uint8_t>(0x28));

DoIPServer server;
std::vector<std::thread> doipReceiver;
bool serverActive = false;
std::unique_ptr<DoIPConnection> connection(nullptr);

/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */
class ExampleDoIPServerModel : public DoIPServerModel {
  public:
    ExampleDoIPServerModel() {
        DOIP_LOG_HIGHLIGHT("Configuring ExampleDoIPServerModel callbacks");

        onCloseConnection = [](IConnectionContext& ctx) noexcept {
            (void)ctx;
            DOIP_LOG_INFO("Connection closed (from ExampleDoIPServerModel)");
        };

        onDiagnosticMessage = [](IConnectionContext& ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
            (void)ctx;
            DOIP_LOG_INFO("Received Diagnostic message (from ExampleDoIPServerModel)", fmt::streamed(msg));

            // Example: Access payload using getPayload()
            auto payload = msg.getPayload();
            if (payload.second >= 3 && payload.first[0] == 0x22 && payload.first[1] == 0xF1 && payload.first[2] == 0x90) {
                DOIP_LOG_INFO(" - Detected Read Data by Identifier for VIN (0xF190) -> send NACK");
                return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
            }

            return std::nullopt;
        };

        onDiagnosticNotification = [](IConnectionContext& ctx, DoIPDiagnosticAck ack) noexcept {
            (void)ctx;
            DOIP_LOG_INFO("Diagnostic ACK/NACK sent (from ExampleDoIPServerModel)", fmt::streamed(ack));
        };
    }
};


void listenUdp();
void listenTcp();

/*
 * Check permantly if udp message was received
 */
void listenUdp() {

    while (serverActive) {
        ssize_t result = server.receiveUdpMessage();
        // If timeout (result == 0), sleep briefly to prevent CPU spinning
        if (result == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

/*
 * Check permantly if tcp message was received
 */
void listenTcp() {

    server.setupTcpSocket();

    while (true) {
        connection = server.waitForTcpConnection<ExampleDoIPServerModel>();

        while (connection->isSocketActive()) {
            connection->receiveTcpMessage();
        }
    }
}

static void ConfigureDoipServer() {
    // VIN needs to have a fixed length of 17 bytes.
    // Shorter VINs will be padded with '0'
    server.setVIN("EXAMPLESERVER");
    server.setLogicalGatewayAddress(LOGICAL_ADDRESS.toUint16());
    server.setGID(0);
    server.setFAR(DoIPFurtherAction::NoFurtherAction);
    server.setEID(0);

    // be more relaxed for testing purposes
    server.setAnnounceInterval(2000);
    server.setAnnounceNum(10);

    // doipserver->setAnnounceNum(tempNum);
    // doipserver->setAnnounceInterval(tempInterval);
}

static void printUsage(const char *progName) {
    cout << "Usage: " << progName << " [OPTIONS]\n";
    cout << "Options:\n";
    cout << "  --loopback    Use loopback (127.0.0.1) for announcements instead of broadcast\n";
    cout << "  --help        Show this help message\n";
}

int main(int argc, char *argv[]) {
    bool useLoopback = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--loopback") {
            useLoopback = true;
            DOIP_LOG_INFO("Loopback mode enabled");
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            cout << "Unknown argument: " << arg << endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Configure logging
    doip::Logger::setLevel(spdlog::level::debug);
    DOIP_LOG_INFO("Starting DoIP Server Example");

    ConfigureDoipServer();

    // Configure server based on mode
    if (useLoopback) {
        server.setAnnouncementMode(true);
    }

    server.setupUdpSocket();

    serverActive = true;
    DOIP_LOG_INFO("Starting UDP and TCP listener threads");
    doipReceiver.push_back(thread(&listenUdp));
    doipReceiver.push_back(thread(&listenTcp));

    server.sendVehicleAnnouncement();
    DOIP_LOG_INFO("Vehicle announcement sent");

    doipReceiver.at(0).join();
    doipReceiver.at(1).join();
    DOIP_LOG_INFO("DoIP Server Example terminated");
    return 0;
}

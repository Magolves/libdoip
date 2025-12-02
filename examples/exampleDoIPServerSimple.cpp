#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "ExampleDoIPServerModel.h"

#include <atomic>
#include <csignal>
#include <iostream>

using namespace doip;
using namespace std;

static const DoIPAddress LOGICAL_ADDRESS(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x28));
static std::atomic<bool> g_shutdownRequested{false};

// Forward declarations
static void signalHandler(int signal);
static std::optional<DoIPServerModel> onConnectionAccepted(DoIPConnection *connection);
static void configureServer(DoIPServer &server);

// Signal handler for graceful shutdown
static void signalHandler(int signal) {
    (void)signal;
    cout << "\nShutdown requested..." << endl;
    g_shutdownRequested.store(true);
}

// Callback invoked when a new connection is accepted
static std::optional<DoIPServerModel> onConnectionAccepted(DoIPConnection *connection) {
    (void)connection;
    cout << "New client connected!" << endl;

    // Create a server model for this connection
    DoIPServerModel model;
    model.serverAddress = LOGICAL_ADDRESS;

    model.onDiagnosticMessage = [](IConnectionContext &ctx, const DoIPMessage &msg) noexcept -> DoIPDiagnosticAck {
        (void)ctx;
        cout << "Received Diagnostic message: " << msg << '\n';

        auto payload = msg.getPayload();
        if (payload.second > 0) {
            cout << "  Service ID: 0x" << hex << setw(2) << setfill('0')
                 << static_cast<int>(payload.first[0]) << dec << '\n';
        }

        // Example logic: NACK if service ID is 0x22 (Read Data by Identifier)
        if (payload.second > 0 && payload.first[0] == 0x22) {
            cout << " - Sending NACK for service ID 0x22\n";
            return DoIPNegativeDiagnosticAck::UnknownTargetAddress;
        }

        return std::nullopt; // Send positive ACK
    };

    model.onDiagnosticNotification = [](IConnectionContext &ctx, DoIPDiagnosticAck ack) noexcept {
        (void)ctx;
        if (ack.has_value()) {
            cout << "Diagnostic NACK sent: " << static_cast<int>(ack.value()) << '\n';
        } else {
            cout << "Diagnostic ACK sent\n";
        }
    };

    model.onCloseConnection = [](IConnectionContext &ctx, DoIPCloseReason reason) noexcept {
        (void)ctx;
        cout << "Connection closed (" << reason << ")" << endl;
    };

    return model;
}

static void configureServer(DoIPServer &server) {
    // VIN needs to have a fixed length of 17 bytes.
    // Shorter VINs will be padded with '0'
    server.setVIN("EXAMPLESERVER1234");
    server.setLogicalGatewayAddress(LOGICAL_ADDRESS.toUint16());
    server.setGID(0x1122334455667788);
    server.setEID(0xAABBCCDDEEFF0011);
    server.setFAR(DoIPFurtherAction::NoFurtherAction);

    // More relaxed timing for testing
    server.setAnnounceInterval(2000);
    server.setAnnounceNum(10);
}

int main(int argc, char *argv[]) {
    bool useLoopback = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--loopback") {
            useLoopback = true;
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [OPTIONS]\n";
            cout << "Options:\n";
            cout << "  --loopback    Use loopback for announcements\n";
            cout << "  --help        Show this help\n";
            return 0;
        } else {
            cerr << "Unknown argument: " << arg << endl;
            return 1;
        }
    }

    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Configure logging
    doip::Logger::setLevel(spdlog::level::info);
    DOIP_LOG_INFO("Starting Simple DoIP Server Example");

    DoIPServer server;
    configureServer(server);

    if (useLoopback) {
        server.setAnnouncementMode(true);
    }

    // Start the server with automatic connection handling
    if (!server.start<ExampleDoIPServerModel>(onConnectionAccepted, true)) {
        DOIP_LOG_ERROR("Failed to start server");
        return 1;
    }

    DOIP_LOG_INFO("Server is running. Press Ctrl+C to stop.");

    // Main thread just waits for shutdown signal
    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Graceful shutdown
    server.stop();
    DOIP_LOG_INFO("Server terminated cleanly");

    return 0;
}

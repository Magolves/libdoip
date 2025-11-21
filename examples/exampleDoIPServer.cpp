#include "DoIPServer.h"
#include "DoIPAddress.h"
#include "Logger.h"

#include<iostream>
#include<iomanip>
#include<thread>
#include<chrono>

using namespace doip;
using namespace std;

static const DoIPAddress LOGICAL_ADDRESS(static_cast<uint8_t>(0x0), static_cast<uint8_t>(0x28));

DoIPServer server;
unique_ptr<DoIPConnection> connection(nullptr);
std::vector<std::thread> doipReceiver;
bool serverActive = false;

void ReceiveFromLibrary(const DoIPAddress& address, const uint8_t* data, size_t length);
DoIPDiagnosticAck DiagnosticMessageReceived(const DoIPMessage &message);

void CloseConnection();
void listenUdp();
void listenTcp();
void ConfigureDoipServer();


DoIPDiagnosticAck DiagnosticMessageReceived(const DoIPMessage &message) {
    cout << "Received Diagnostic message" << message << '\n';

    return std::nullopt;
}

/**
 * Closes the connection of the server by ending the listener threads
 */
void CloseConnection() {
    cout << "Connection closed" << endl;
    //serverActive = false;
}

/*
 * Check permantly if udp message was received
 */
void listenUdp() {

    while(serverActive) {
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

    while(true) {
        connection = server.waitForTcpConnection();

        DoIPServerModel model;
        model.serverAddress = LOGICAL_ADDRESS;
        model.onCloseConnection = CloseConnection;
        model.onDiagnosticMessage = DiagnosticMessageReceived;
        model.onDiagnosticNotification = [](DoIPDiagnosticAck ack) noexcept {
            (void)ack;
        };

        connection->setServerModel(model);

         while(connection->isSocketActive()) {
             connection->receiveTcpMessage();
         }
    }
}

void ConfigureDoipServer() {
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

static void printUsage(const char* progName) {
    cout << "Usage: " << progName << " [OPTIONS]\n";
    cout << "Options:\n";
    cout << "  --loopback    Use loopback (127.0.0.1) for announcements instead of broadcast\n";
    cout << "  --help        Show this help message\n";
}

int main(int argc, char* argv[]) {
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
        server.setAnnouncementMode(true);  // We'll add this method
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

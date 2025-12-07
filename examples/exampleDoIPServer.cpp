#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "DoIPServer.h"

#include "ExampleDoIPServerModel.h"

using namespace doip;
using namespace std;

static const DoIPAddress LOGICAL_ADDRESS(0x0028);

DoIPServer server;
std::vector<std::thread> doipReceiver;
bool serverActive = false;
std::unique_ptr<DoIPConnection> tcpConnection(nullptr);

void listenUdp();
void listenTcp();

/*
 * Check permantly if udp message was received
 */
void listenUdp() {
    LOG_UDP_INFO("UDP listener thread started");
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
    LOG_UDP_INFO("TCP listener thread started");

    while (true) {
        tcpConnection = server.waitForTcpConnection<ExampleDoIPServerModel>();

        while (tcpConnection->isSocketActive()) {
            tcpConnection->receiveTcpMessage();
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
            LOG_DOIP_INFO("Loopback mode enabled");
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
    LOG_DOIP_INFO("Starting DoIP Server Example");

    ConfigureDoipServer();

    // Configure server based on mode
    if (useLoopback) {
        server.setAnnouncementMode(true);
    }

    if (!server.setupUdpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up UDP socket");
        return 1;
    }

    if (!server.setupTcpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up TCP socket");
        return 1;
    }

    serverActive = true;
    LOG_DOIP_INFO("Starting UDP and TCP listener threads");
    doipReceiver.push_back(thread(&listenUdp));
    doipReceiver.push_back(thread(&listenTcp));

    server.sendVehicleAnnouncement();
    LOG_DOIP_INFO("Vehicle announcement sent");

    doipReceiver.at(0).join();
    doipReceiver.at(1).join();
    LOG_DOIP_INFO("DoIP Server Example terminated");
    return 0;
}

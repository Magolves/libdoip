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

std::unique_ptr<DoIPServer> server;
std::vector<std::thread> doipReceiver;
bool serverActive = false;
std::unique_ptr<DoIPConnection> tcpConnection(nullptr);

void listenTcp();

/*
 * Check permantly if tcp message was received
 */
void listenTcp() {
    LOG_UDP_INFO("TCP listener thread started");

    while (true) {
        tcpConnection = server->waitForTcpConnection<ExampleDoIPServerModel>();

        while (tcpConnection->isSocketActive()) {
            tcpConnection->receiveTcpMessage();
        }
    }
}

// Default example settings are applied in main when building ServerConfig

static void printUsage(const char *progName) {
    cout << "Usage: " << progName << " [OPTIONS]\n";
    cout << "Options:\n";
    cout << "  --loopback    Use loopback (127.0.0.1) for announcements instead of broadcast\n";
    cout << "  --daemonize / --no-daemonize  Enable or disable daemon mode (default: enabled)\n";
    cout << "  --eid <6chars>  Set EID (6 ASCII chars)\n";
    cout << "  --gid <6chars>  Set GID (6 ASCII chars)\n";
    cout << "  --vin <17chars> Set VIN (17 ASCII chars)\n";
    cout << "  --logical-address <hex|dec> Set logical gateway address (default: 0x0E00)\n";
    cout << "  --help        Show this help message\n";
}

int main(int argc, char *argv[]) {
    bool useLoopback = false;
    bool daemonize = false;
    std::string eid_str;
    std::string gid_str;
    std::string vin_str = "EXAMPLESERVER";
    std::string logical_addr_str;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--loopback") {
            useLoopback = true;
            LOG_DOIP_INFO("Loopback mode enabled");
        } else if (arg == "--daemonize") {
            daemonize = true;
            LOG_DOIP_INFO("Daemonize enabled");
        } else if (arg == "--no-daemonize") {
            daemonize = false;
            LOG_DOIP_INFO("Daemonize disabled");
        } else if (arg == "--eid" && i + 1 < argc) {
            eid_str = argv[++i];
        } else if (arg == "--gid" && i + 1 < argc) {
            gid_str = argv[++i];
        } else if (arg == "--vin" && i + 1 < argc) {
            vin_str = argv[++i];
        } else if (arg == "--logical-address" && i + 1 < argc) {
            logical_addr_str = argv[++i];
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

    // Build server config from example settings and CLI
    doip::ServerConfig cfg;
    cfg.loopback = useLoopback;
    cfg.daemonize = daemonize;
    // TODO: Use CLI11 or similar for argument parsing
    if (!vin_str.empty()) cfg.vin = DoIPVIN(vin_str);
    if (!eid_str.empty()) cfg.eid = DoIPEID(eid_str);
    if (!gid_str.empty()) cfg.gid = DoIPGID(gid_str);
    if (!logical_addr_str.empty()) {
        // parse hex (0x...) or decimal
        try {
            size_t pos = 0;
            unsigned long val = 0;
            if (logical_addr_str.rfind("0x", 0) == 0 || logical_addr_str.rfind("0X", 0) == 0) {
                val = std::stoul(logical_addr_str, &pos, 16);
            } else {
                val = std::stoul(logical_addr_str, &pos, 0);
            }
            cfg.logicalAddress = DoIPAddress(static_cast<uint16_t>(val & 0xFFFF));
        } catch (...) {
            LOG_DOIP_WARN("Failed to parse logical address '{}', using default 0x0E00", logical_addr_str);
        }
    } else {
        cfg.logicalAddress = LOGICAL_ADDRESS;
    }

    server = std::make_unique<DoIPServer>(cfg);
    // Apply defaults used previously in example
    server->setFurtherActionRequired(DoIPFurtherAction::NoFurtherAction);
    server->setAnnounceInterval(2000);
    server->setAnnounceNum(10);

    if (!server->setupUdpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up UDP socket");
        return 1;
    }

    if (!server->setupTcpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up TCP socket");
        return 1;
    }

    serverActive = true;

    // TODO:: Add signal handler
    while(server->isRunning()) {
        sleep(1);
    }
    doipReceiver.push_back(thread(&listenTcp));
    LOG_DOIP_INFO("Starting TCP listener threads");

    doipReceiver.at(0).join();
    LOG_DOIP_INFO("DoIP Server Example terminated");
    return 0;
}

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "DoIPServer.h"

#include "ExampleDoIPServerModel.h"
#include "cli/ServerConfigCLI.h"

using namespace doip;
using namespace std;

static const DoIPAddress LOGICAL_ADDRESS(0x0028);

std::unique_ptr<DoIPServer> server;
std::vector<std::thread> doipReceiver;
bool serverActive = false;
std::unique_ptr<DoIPConnection> tcpConnection(nullptr);

void listenTcp();

/*
 * Check permanently if tcp message was received
 */
void listenTcp() {
    LOG_UDP_INFO("TCP listener thread started");

    while (true) {
        tcpConnection = server->waitForTcpConnection(std::make_unique<ExampleDoIPServerModel>());

        while (tcpConnection->isSocketActive()) {
            tcpConnection->receiveTcpMessage();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int main(int argc, char *argv[]) {
    // Parse command line arguments using ServerConfigCLI
    doip::ServerConfig cfg;
    cli::ServerConfigCLI cli;
    try {
        cfg = cli.parse_and_build(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Configure logging
    doip::Logger::setLevel(spdlog::level::debug);
    LOG_DOIP_INFO("Starting DoIP Server Example");

    server = std::make_unique<DoIPServer>(cfg);
    // Apply defaults used previously in example
    server->setFurtherActionRequired(DoIPFurtherAction::NoFurtherAction);
    server->setAnnounceInterval(2000);
    server->setAnnounceNum(10);

    if (!server->setupUdpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up UDP socket");
        return 1;
    }

    if (!server->setupTcpSocket([](){ return std::make_unique<ExampleDoIPServerModel>(); })) {
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

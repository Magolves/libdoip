#include "DoIPClient.h"
#include "DoIPMessage.h"
#include "Logger.h"

#include <iomanip>
#include <iostream>
#include <thread>

using namespace doip;
using namespace std;

DoIPClient client;

static void printUsage(const char *progName) {
    cout << "Usage: " << progName << " [OPTIONS]\n";
    cout << "Options:\n";
    cout << "  --loopback            Use loopback (127.0.0.1) instead of multicast\n";
    cout << "  --server <ip>         Connect to specific server IP\n";
    cout << "  --help                Show this help message\n";
}

int main(int argc, char *argv[]) {
    std::cerr << "The client code does not work currently - use at your own risk!\n";

    string serverAddress = "224.0.0.2"; // Default multicast address

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--loopback") {
            serverAddress = "127.0.0.1";
            LOG_DOIP_INFO("Loopback mode enabled - using 127.0.0.1");
        } else if (arg == "--server" && i + 1 < argc) {
            serverAddress = argv[++i];
            LOG_DOIP_INFO("Using custom server address: {}", serverAddress);
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            cout << "Unknown argument: " << arg << endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    LOG_DOIP_INFO("Starting DoIP Client");

    // Start UDP connections (don't start TCP yet)
    client.startUdpConnection();
    client.startAnnouncementListener(); // Listen for Vehicle Announcements on port 13401

    // Listen for Vehicle Announcements first
    LOG_DOIP_INFO("Listening for Vehicle Announcements...");
    if (!client.receiveVehicleAnnouncement()) {
        LOG_DOIP_WARN("No Vehicle Announcement received");
        return EXIT_FAILURE;
    }

    client.printVehicleInformationResponse();

    // Send Vehicle Identification Request to configured address
    if (client.sendVehicleIdentificationRequest(serverAddress.c_str()) > 0) {
        LOG_DOIP_INFO("Vehicle Identification Request sent successfully");
        client.receiveUdpMessage();
    }

    // Now start TCP connection for diagnostic communication
    LOG_DOIP_INFO("Discovery complete, closing UDP connections");
    client.closeUdpConnection();
    return 0;
}

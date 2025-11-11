#include "DoIPClient_h.h"
#include "Logger.h"

#include<iostream>
#include<iomanip>
#include<thread>

using namespace doip;
using namespace std;


DoIPClient client;


int main() {
    DOIP_LOG_INFO("Starting DoIP Client");

    // Start UDP connections (don't start TCP yet)
    client.startUdpConnection();
    client.startAnnouncementListener(); // Listen for Vehicle Announcements on port 13401

    // Listen for Vehicle Announcements first
    DOIP_LOG_INFO("Listening for Vehicle Announcements...");
    client.receiveVehicleAnnouncement();

    // Send Vehicle Identification Request to DoIP multicast address
    if (client.sendVehicleIdentificationRequest("224.0.0.2") > 0) {
        DOIP_LOG_INFO("Vehicle Identification Request sent successfully");
        client.receiveUdpMessage();
    }

    // Now start TCP connection for diagnostic communication
    DOIP_LOG_INFO("Starting TCP connection for diagnostic messages");
    client.startTcpConnection();

    if (client.sendRoutingActivationRequest() < 0) {
        std::cerr << "sendRoutingActivationRequest Failed";

        exit(EXIT_FAILURE);
    }

    client.receiveMessage();

    client.closeTcpConnection();
    client.closeUdpConnection();
}

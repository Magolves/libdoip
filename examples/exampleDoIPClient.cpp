#include "DoIPClient_h.h"

#include<iostream>
#include<iomanip>
#include<thread>

using namespace doip;
using namespace std;


DoIPClient client;


int main() {
    client.startUdpConnection();
    client.startTcpConnection();

    auto retries = 10;
    while(retries > 0 && !client.getConnected()) {
        sleep(1);
        std::cout << "Waiting for server...\n";
        --retries;
    }

    if (client.sendRoutingActivationRequest() < 0) {
        std::cerr << "sendRoutingActivationRequest Failed";
        exit(EXIT_FAILURE);
    }

    client.receiveMessage();

    client.closeTcpConnection();
    client.closeUdpConnection();
}

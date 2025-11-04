#include "DoIPClient_h.h"

using namespace doip;

/*
 *Set up the connection between client and server
 */
void DoIPClient::startTcpConnection() {

    const char *ipAddr = "127.0.0.1";
    bool connectedFlag = false;
    _sockFd = socket(AF_INET, SOCK_STREAM, 0);

    if (_sockFd >= 0) {
        std::cout << "Client TCP-Socket created successfully" << '\n';

        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(_serverPortNr);
        inet_aton(ipAddr, &(_serverAddr.sin_addr));

        while (!connectedFlag) {
            _connected = connect(_sockFd, reinterpret_cast<struct sockaddr *>(&_serverAddr), sizeof(_serverAddr));
            if (_connected != -1) {
                connectedFlag = true;
                std::cout << "Connection to server established" << '\n';
            }
        }
    }
}

void DoIPClient::startUdpConnection() {

    _sockFd_udp = socket(AF_INET, SOCK_DGRAM, 0);

    if (_sockFd_udp >= 0) {
        std::cout << "Client-UDP-Socket created successfully" << '\n';

        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(_serverPortNr);
        _serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        _clientAddr.sin_family = AF_INET;
        _clientAddr.sin_port = htons(_serverPortNr);
        _clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // binds the socket to any IP DoIPAddress and the Port Number 13400
        bind(_sockFd_udp, reinterpret_cast<struct sockaddr *>(&_clientAddr), sizeof(_clientAddr));
    }
}

/*
 * closes the client-socket
 */
void DoIPClient::closeTcpConnection() {
    close(_sockFd);
}

void DoIPClient::closeUdpConnection() {
    close(_sockFd_udp);
}

void DoIPClient::reconnectServer() {
    closeTcpConnection();
    startTcpConnection();
}

/*
 *Build the Routing-Activation-Request for server
 */
const DoIPRequest DoIPClient::buildRoutingActivationRequest() {

    //std::pair<int, uint8_t *> *rareqWithLength = new std::pair<int, uint8_t *>();
    size_t rareqLength = 15;
    uint8_t *rareq = new uint8_t[rareqLength];

    // Generic Header
    rareq[0] = 0x02; // Protocol Version
    rareq[1] = 0xFD; // Inverse Protocol Version
    rareq[2] = 0x00; // Payload-Type
    rareq[3] = 0x05;
    rareq[4] = 0x00; // Payload-Length
    rareq[5] = 0x00;
    rareq[6] = 0x00;
    rareq[7] = 0x07;

    // Payload-Type specific message-content
    rareq[8] = 0x0E; // Source DoIPAddress
    rareq[9] = 0x00;
    rareq[10] = 0x00; // Activation-Type
    rareq[11] = 0x00; // Reserved ISO(default)
    rareq[12] = 0x00;
    rareq[13] = 0x00;
    rareq[14] = 0x00;

    // rareqWithLength->first = rareqLength;
    // rareqWithLength->second = rareq;

    return std::make_pair(rareqLength, rareq);
}

/*
 * Send the builded request over the tcp-connection to server
 */
ssize_t DoIPClient::sendRoutingActivationRequest() {

    const DoIPRequest rareqWithLength = buildRoutingActivationRequest();
    return write(_sockFd, rareqWithLength.second, rareqWithLength.first);
}

/**
 * Sends a diagnostic message to the server
 * @param targetAddress     the address of the ecu which should receive the message
 * @param userData          data that will be given to the ecu
 * @param userDataLength    length of userData
 */
ssize_t DoIPClient::sendDiagnosticMessage(const DoIPAddress &targetAddress, uint8_t *userData, size_t userDataLength) {
    DoIPAddress sourceAddress(0x0E, 0x00);
    uint8_t *message = createDiagnosticMessage(sourceAddress, targetAddress, userData, userDataLength);

    return write(_sockFd, message, _GenericHeaderLength + _DiagnosticMessageMinimumLength + userDataLength);
}

/**
 * Sends a alive check response containing the clients source address to the server
 */
ssize_t DoIPClient::sendAliveCheckResponse() {
    size_t responseLength = 2;
    uint8_t *message = createGenericHeader(PayloadType::ALIVECHECKRESPONSE, responseLength);
    message[8] = m_sourceAddress.hsb();
    message[9] = m_sourceAddress.lsb();
    return write(_sockFd, message, _GenericHeaderLength + responseLength);
}

/*
 * Receive a message from server
 */
void DoIPClient::receiveMessage() {

    int bytesRead = recv(_sockFd, _receivedData, _maxDataSize, 0);

    if (!bytesRead) // if server is disconnected from client; client gets empty messages
    {
        emptyMessageCounter++;

        if (emptyMessageCounter == 5) {
            std::cout << "Received to many empty messages. Reconnect TCP connection" << '\n';
            emptyMessageCounter = 0;
            reconnectServer();
        }
        return;
    }

    printf("Client received: ");
    for (int i = 0; i < bytesRead; i++) {
        printf("0x%02X ", _receivedData[i]);
    }
    printf("\n ");

    GenericHeaderAction action = parseGenericHeader(_receivedData, bytesRead);

    if (action.type == PayloadType::DIAGNOSTICPOSITIVEACK || action.type == PayloadType::DIAGNOSTICNEGATIVEACK) {
        switch (action.type) {
        case PayloadType::DIAGNOSTICPOSITIVEACK: {
            std::cout << "Client received diagnostic message positive ack with code: ";
            printf("0x%02X ", _receivedData[12]);
            break;
        }
        case PayloadType::DIAGNOSTICNEGATIVEACK: {
            std::cout << "Client received diagnostic message negative ack with code: ";
            printf("0x%02X ", _receivedData[12]);
            break;
        }
        default: {
            std::cerr << "not handled payload type occured in receiveMessage()" << '\n';
            break;
        }
        }
        std::cout << '\n';
    }
}

void DoIPClient::receiveUdpMessage() {

    unsigned int length = sizeof(_clientAddr);

    int bytesRead;
    bytesRead = recvfrom(_sockFd_udp, _receivedData, _maxDataSize, 0, reinterpret_cast<struct sockaddr *>(&_clientAddr), &length);

    if (PayloadType::VEHICLEIDENTRESPONSE == parseGenericHeader(_receivedData, bytesRead).type) {
        parseVIResponseInformation(_receivedData);
    }
}

const DoIPRequest DoIPClient::buildVehicleIdentificationRequest() {

    //std::pair<int, uint8_t *> *rareqWithLength = new std::pair<int, uint8_t *>();
    size_t rareqLength = 8;
    uint8_t *rareq = new uint8_t[rareqLength];

    // Generic Header
    rareq[0] = 0x02; // Protocol Version
    rareq[1] = 0xFD; // Inverse Protocol Version
    rareq[2] = 0x00; // Payload-Type
    rareq[3] = 0x01;
    rareq[4] = 0x00; // Payload-Length
    rareq[5] = 0x00;
    rareq[6] = 0x00;
    rareq[7] = 0x00;

    // rareqWithLength->first = rareqLength;
    // rareqWithLength->second = rareq;

    return std::make_pair(rareqLength, rareq);
}

ssize_t DoIPClient::sendVehicleIdentificationRequest(const char *inet_address) {

    int setAddressError = inet_aton(inet_address, &(_serverAddr.sin_addr));

    if (setAddressError != 0) {
        std::cout << "DoIPAddress set succesfully" << '\n';
    } else {
        std::cout << "Could not set DoIPAddress. Try again" << '\n';
    }

    int socketError = setsockopt(_sockFd_udp, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    if (socketError == 0) {
        std::cout << "Broadcast Option set successfully" << '\n';
    }

    const DoIPRequest rareqWithLength = buildVehicleIdentificationRequest();

    ssize_t bytesSent = sendto(_sockFd_udp, rareqWithLength.second, rareqWithLength.first, 0, reinterpret_cast<struct sockaddr *>(&_serverAddr), sizeof(_serverAddr));

    if (bytesSent > 0) {
        std::cout << "Sending Vehicle Identification Request" << '\n';
    }

    return bytesSent;
}

/**
 * Sets the source address for this client
 * @param address   source address for the client
 */
void DoIPClient::setSourceAddress(const DoIPAddress &address) {
    m_sourceAddress = address;
}

/*
 * Getter for _sockFD
 */
int DoIPClient::getSockFd() {
    return _sockFd;
}

/*
 * Getter for _connected
 */
int DoIPClient::getConnected() {
    return _connected;
}

void DoIPClient::parseVIResponseInformation(uint8_t *data) {

    // VIN
    int j = 0;
    for (int i = 8; i <= 24; i++) {
        VINResult[j] = data[i];
        j++;
    }

    // Logical Adress
    LogicalAddressResult.update(data, 25);

    // EID
    j = 0;
    for (int i = 27; i <= 32; i++) {
        EIDResult[j] = data[i];
        j++;
    }

    // GID
    j = 0;
    for (int i = 33; i <= 38; i++) {
        GIDResult[j] = data[i];
        j++;
    }

    // FurtherActionRequest
    FurtherActionReqResult = data[39];
}

void DoIPClient::displayVIResponseInformation() {
    // output VIN
    std::cout << "VIN: ";
    for (int i = 0; i < 17; i++) {
        std::cout << +VINResult[i];
    }
    std::cout << '\n';

    // output LogicalAddress
    std::cout << "LogicalAddress: ";
    std::cout << LogicalAddressResult;
    std::cout << '\n';

    // output EID
    std::cout << "EID: ";
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << EIDResult[i] << std::dec;
    }
    std::cout << '\n';

    // output GID
    std::cout << "GID: ";
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << GIDResult[i] << std::dec;
    }
    std::cout << '\n';

    // output FurtherActionRequest
    std::cout << "FurtherActionRequest: ";
    std::cout << std::hex << std::setfill('0') << std::setw(2) << FurtherActionReqResult << std::dec;

    std::cout << '\n';
}

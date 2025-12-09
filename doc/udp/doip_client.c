#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdbool.h>

#define DOIP_UDP_DISCOVERY_PORT 13400
#define DOIP_UDP_TEST_EQUIPMENT_PORT 13401
#define DOIP_PROTOCOL_VERSION 0x04
#define DOIP_INVERSE_PROTOCOL_VERSION 0xFB

// DoIP Payload Types
#define VEHICLE_IDENTIFICATION_REQUEST 0x0001
#define VEHICLE_IDENTIFICATION_RESPONSE 0x0004

// Discovered vehicle information
typedef struct {
    char vin[18];
    uint16_t logical_address;
    uint8_t eid[6];
    uint8_t gid[6];
    char ip_address[INET_ADDRSTRLEN];
    uint16_t port;
} VehicleInfo;

// Create DoIP header
void create_doip_header(uint8_t *buffer, uint16_t payload_type, uint32_t payload_length) {
    buffer[0] = DOIP_PROTOCOL_VERSION;
    buffer[1] = DOIP_INVERSE_PROTOCOL_VERSION;
    buffer[2] = (payload_type >> 8) & 0xFF;
    buffer[3] = payload_type & 0xFF;
    buffer[4] = (payload_length >> 24) & 0xFF;
    buffer[5] = (payload_length >> 16) & 0xFF;
    buffer[6] = (payload_length >> 8) & 0xFF;
    buffer[7] = payload_length & 0xFF;
}

// Create Vehicle Identification Request
int create_vehicle_identification_request(uint8_t *buffer) {
    create_doip_header(buffer, VEHICLE_IDENTIFICATION_REQUEST, 0);
    return 8; // Header only, no payload
}

// Parse DoIP header
bool parse_doip_header(const uint8_t *buffer, size_t length, uint16_t *payload_type, uint32_t *payload_length) {
    if (length < 8) {
        return false;
    }
    
    if (buffer[0] != DOIP_PROTOCOL_VERSION || buffer[1] != DOIP_INVERSE_PROTOCOL_VERSION) {
        printf("[CLIENT] Invalid DoIP protocol version\n");
        return false;
    }
    
    *payload_type = (buffer[2] << 8) | buffer[3];
    *payload_length = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    
    return true;
}

// Parse Vehicle Identification Response
bool parse_vehicle_identification_response(const uint8_t *buffer, size_t length, VehicleInfo *info) {
    if (length < 41) { // 8 (header) + 33 (minimum payload)
        printf("[CLIENT] Message too short for Vehicle Identification Response\n");
        return false;
    }
    
    int offset = 8; // Skip header
    
    // VIN (17 bytes)
    memcpy(info->vin, buffer + offset, 17);
    info->vin[17] = '\0';
    offset += 17;
    
    // Logical Address (2 bytes)
    info->logical_address = (buffer[offset] << 8) | buffer[offset + 1];
    offset += 2;
    
    // EID (6 bytes)
    memcpy(info->eid, buffer + offset, 6);
    offset += 6;
    
    // GID (6 bytes)
    memcpy(info->gid, buffer + offset, 6);
    offset += 6;
    
    return true;
}

// Print vehicle information
void print_vehicle_info(const VehicleInfo *info) {
    printf("\n[CLIENT] ========== Vehicle Information ==========\n");
    printf("[CLIENT] VIN: %s\n", info->vin);
    printf("[CLIENT] Logical Address: 0x%04X\n", info->logical_address);
    printf("[CLIENT] EID: %02X:%02X:%02X:%02X:%02X:%02X\n",
           info->eid[0], info->eid[1], info->eid[2],
           info->eid[3], info->eid[4], info->eid[5]);
    printf("[CLIENT] GID: %02X:%02X:%02X:%02X:%02X:%02X\n",
           info->gid[0], info->gid[1], info->gid[2],
           info->gid[3], info->gid[4], info->gid[5]);
    printf("[CLIENT] Server IP: %s\n", info->ip_address);
    printf("[CLIENT] Server Port: %d\n", info->port);
    printf("[CLIENT] ============================================\n\n");
}

int main(int argc, char *argv[]) {
    bool use_loopback = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--loopback") == 0) {
            use_loopback = true;
        }
    }
    
    printf("[CLIENT] Starting DoIP Client\n");
    printf("[CLIENT] Mode: %s\n", use_loopback ? "Loopback" : "Broadcast");
    
    // Create socket for sending requests
    int request_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (request_sock < 0) {
        perror("[CLIENT] Failed to create request socket");
        return 1;
    }
    
    // Create socket for receiving announcements
    int announcement_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (announcement_sock < 0) {
        perror("[CLIENT] Failed to create announcement socket");
        close(request_sock);
        return 1;
    }
    
    // Enable SO_REUSEADDR for announcement socket
    int reuse = 1;
    setsockopt(announcement_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Enable broadcast reception
    int broadcast = 1;
    if (setsockopt(announcement_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("[CLIENT] Failed to enable broadcast reception");
    }
    
    // Bind announcement socket to port 13401
    struct sockaddr_in announcement_addr;
    memset(&announcement_addr, 0, sizeof(announcement_addr));
    announcement_addr.sin_family = AF_INET;
    announcement_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    announcement_addr.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_PORT);
    
    if (bind(announcement_sock, (struct sockaddr *)&announcement_addr, sizeof(announcement_addr)) < 0) {
        perror("[CLIENT] Failed to bind announcement socket");
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    printf("[CLIENT] Announcement socket bound to 0.0.0.0:%d\n", DOIP_UDP_TEST_EQUIPMENT_PORT);
    
    // Set timeout for announcement socket
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(announcement_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Listen for Vehicle Announcement
    printf("[CLIENT] Listening for Vehicle Announcements...\n");
    
    uint8_t buffer[512];
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    
    ssize_t received = recvfrom(announcement_sock, buffer, sizeof(buffer), 0,
                               (struct sockaddr *)&server_addr, &server_len);
    
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[CLIENT] Timeout: No Vehicle Announcement received\n");
        } else {
            perror("[CLIENT] Error receiving announcement");
        }
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, server_ip, sizeof(server_ip));
    printf("[CLIENT] Received announcement: %zd bytes from %s:%d\n",
           received, server_ip, ntohs(server_addr.sin_port));
    
    // Parse the announcement
    uint16_t payload_type;
    uint32_t payload_length;
    
    if (!parse_doip_header(buffer, received, &payload_type, &payload_length)) {
        printf("[CLIENT] Failed to parse DoIP header\n");
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    if (payload_type != VEHICLE_IDENTIFICATION_RESPONSE) {
        printf("[CLIENT] Unexpected payload type: 0x%04X\n", payload_type);
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    VehicleInfo vehicle_info;
    memset(&vehicle_info, 0, sizeof(vehicle_info));
    
    if (!parse_vehicle_identification_response(buffer, received, &vehicle_info)) {
        printf("[CLIENT] Failed to parse Vehicle Identification Response\n");
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    // Store server information
    strncpy(vehicle_info.ip_address, server_ip, sizeof(vehicle_info.ip_address));
    vehicle_info.port = DOIP_UDP_DISCOVERY_PORT; // Requests go to port 13400
    
    print_vehicle_info(&vehicle_info);
    
    // Now send a Vehicle Identification Request to the discovered server
    printf("[CLIENT] Sending Vehicle Identification Request to %s:%d\n",
           vehicle_info.ip_address, vehicle_info.port);
    
    struct sockaddr_in request_addr;
    memset(&request_addr, 0, sizeof(request_addr));
    request_addr.sin_family = AF_INET;
    request_addr.sin_port = htons(vehicle_info.port);
    inet_pton(AF_INET, vehicle_info.ip_address, &request_addr.sin_addr);
    
    uint8_t request[8];
    int request_len = create_vehicle_identification_request(request);
    
    ssize_t sent = sendto(request_sock, request, request_len, 0,
                         (struct sockaddr *)&request_addr, sizeof(request_addr));
    
    if (sent < 0) {
        perror("[CLIENT] Failed to send request");
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    printf("[CLIENT] Sent Vehicle Identification Request: %zd bytes\n", sent);
    
    // Wait for response
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(request_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in response_addr;
    socklen_t response_len = sizeof(response_addr);
    
    received = recvfrom(request_sock, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)&response_addr, &response_len);
    
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[CLIENT] Timeout: No response received\n");
        } else {
            perror("[CLIENT] Error receiving response");
        }
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    char response_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &response_addr.sin_addr, response_ip, sizeof(response_ip));
    printf("[CLIENT] Received response: %zd bytes from %s:%d\n",
           received, response_ip, ntohs(response_addr.sin_port));
    
    // Parse the response
    if (!parse_doip_header(buffer, received, &payload_type, &payload_length)) {
        printf("[CLIENT] Failed to parse response header\n");
        close(request_sock);
        close(announcement_sock);
        return 1;
    }
    
    if (payload_type == VEHICLE_IDENTIFICATION_RESPONSE) {
        VehicleInfo response_info;
        memset(&response_info, 0, sizeof(response_info));
        
        if (parse_vehicle_identification_response(buffer, received, &response_info)) {
            strncpy(response_info.ip_address, response_ip, sizeof(response_info.ip_address));
            response_info.port = ntohs(response_addr.sin_port);
            
            printf("[CLIENT] Vehicle Identification Response received:\n");
            print_vehicle_info(&response_info);
        }
    }
    
    // Cleanup
    close(request_sock);
    close(announcement_sock);
    
    printf("[CLIENT] Discovery complete\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#define DOIP_UDP_DISCOVERY_PORT 13400
#define DOIP_UDP_TEST_EQUIPMENT_PORT 13401
#define DOIP_PROTOCOL_VERSION 0x04
#define DOIP_INVERSE_PROTOCOL_VERSION 0xFB

// DoIP Payload Types
#define VEHICLE_IDENTIFICATION_REQUEST 0x0001
#define VEHICLE_IDENTIFICATION_RESPONSE 0x0004

// Server configuration
typedef struct {
    char vin[17];
    uint16_t logical_address;
    uint8_t eid[6];
    uint8_t gid[6];
    bool use_loopback;
} ServerConfig;

// Global server state
static bool server_running = true;
static int udp_sock = -1;

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

// Create Vehicle Identification Response
int create_vehicle_identification_response(uint8_t *buffer, const ServerConfig *config) {
    uint32_t payload_length = 33; // VIN(17) + LogAddr(2) + EID(6) + GID(6) + FAR(1) + VIN/GID_sync(1)
    
    create_doip_header(buffer, VEHICLE_IDENTIFICATION_RESPONSE, payload_length);
    
    int offset = 8;
    
    // VIN (17 bytes)
    memcpy(buffer + offset, config->vin, 17);
    offset += 17;
    
    // Logical Address (2 bytes)
    buffer[offset++] = (config->logical_address >> 8) & 0xFF;
    buffer[offset++] = config->logical_address & 0xFF;
    
    // EID (6 bytes)
    memcpy(buffer + offset, config->eid, 6);
    offset += 6;
    
    // GID (6 bytes)
    memcpy(buffer + offset, config->gid, 6);
    offset += 6;
    
    // Further Action Required (1 byte)
    buffer[offset++] = 0x00;
    
    // VIN/GID sync status (1 byte) - optional, set to 0x00
    buffer[offset++] = 0x00;
    
    return offset; // Total message length
}

// Parse DoIP header
bool parse_doip_header(const uint8_t *buffer, size_t length, uint16_t *payload_type, uint32_t *payload_length) {
    if (length < 8) {
        return false;
    }
    
    if (buffer[0] != DOIP_PROTOCOL_VERSION || buffer[1] != DOIP_INVERSE_PROTOCOL_VERSION) {
        printf("Invalid DoIP protocol version\n");
        return false;
    }
    
    *payload_type = (buffer[2] << 8) | buffer[3];
    *payload_length = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    
    return true;
}

// Send Vehicle Announcement
void send_vehicle_announcement(const ServerConfig *config) {
    uint8_t buffer[256];
    int msg_len = create_vehicle_identification_response(buffer, config);
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_PORT);
    
    const char *dest_ip;
    if (config->use_loopback) {
        dest_ip = "127.0.0.1";
        inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);
    } else {
        dest_ip = "255.255.255.255";
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        
        // Enable broadcast
        int broadcast = 1;
        setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    }
    
    ssize_t sent = sendto(udp_sock, buffer, msg_len, 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    if (sent > 0) {
        printf("[SERVER] Sent Vehicle Announcement: %zd bytes to %s:%d\n",
               sent, dest_ip, DOIP_UDP_TEST_EQUIPMENT_PORT);
    } else {
        perror("[SERVER] Failed to send announcement");
    }
}

// UDP Listener Thread
void *udp_listener_thread(void *arg) {
    ServerConfig *config = (ServerConfig *)arg;
    uint8_t buffer[512];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    printf("[SERVER] UDP listener thread started\n");
    
    while (server_running) {
        ssize_t received = recvfrom(udp_sock, buffer, sizeof(buffer), 0,
                                   (struct sockaddr *)&client_addr, &client_len);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout, continue
                continue;
            }
            if (server_running) {
                perror("[SERVER] recvfrom error");
            }
            break;
        }
        
        if (received > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            printf("[SERVER] Received %zd bytes from %s:%d\n",
                   received, client_ip, ntohs(client_addr.sin_port));
            
            uint16_t payload_type;
            uint32_t payload_length;
            
            if (parse_doip_header(buffer, received, &payload_type, &payload_length)) {
                printf("[SERVER] Payload Type: 0x%04X\n", payload_type);
                
                if (payload_type == VEHICLE_IDENTIFICATION_REQUEST) {
                    printf("[SERVER] Vehicle Identification Request received\n");
                    
                    // Send response back to client
                    uint8_t response[256];
                    int resp_len = create_vehicle_identification_response(response, config);
                    
                    ssize_t sent = sendto(udp_sock, response, resp_len, 0,
                                        (struct sockaddr *)&client_addr, client_len);
                    
                    if (sent > 0) {
                        printf("[SERVER] Sent Vehicle Identification Response: %zd bytes to %s:%d\n",
                               sent, client_ip, ntohs(client_addr.sin_port));
                    } else {
                        perror("[SERVER] Failed to send response");
                    }
                }
            }
        }
    }
    
    printf("[SERVER] UDP listener thread stopped\n");
    return NULL;
}

// Announcement Thread
void *announcement_thread(void *arg) {
    ServerConfig *config = (ServerConfig *)arg;
    
    printf("[SERVER] Announcement thread started\n");
    
    // Send 5 announcements with 2 second interval
    for (int i = 0; i < 5 && server_running; i++) {
        send_vehicle_announcement(config);
        sleep(2);
    }
    
    printf("[SERVER] Announcement thread stopped\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    bool use_loopback = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--loopback") == 0) {
            use_loopback = true;
        }
    }
    
    // Configure server
    ServerConfig config = {
        .vin = "EXAMPLESERVER0000",
        .logical_address = 0x0028,
        .eid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .gid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .use_loopback = use_loopback
    };
    
    printf("[SERVER] Starting DoIP Server\n");
    printf("[SERVER] Mode: %s\n", use_loopback ? "Loopback" : "Broadcast");
    printf("[SERVER] VIN: %s\n", config.vin);
    printf("[SERVER] Logical Address: 0x%04X\n", config.logical_address);
    
    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Set socket to non-blocking with timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Enable SO_REUSEADDR
    int reuse = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind socket to port 13400
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
    
    if (bind(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(udp_sock);
        return 1;
    }
    
    printf("[SERVER] Socket bound to 0.0.0.0:%d\n", DOIP_UDP_DISCOVERY_PORT);
    
    // Start threads
    pthread_t listener_tid, announcement_tid;
    
    pthread_create(&listener_tid, NULL, udp_listener_thread, &config);
    pthread_create(&announcement_tid, NULL, announcement_thread, &config);
    
    // Wait for announcement thread to complete
    pthread_join(announcement_tid, NULL);
    
    // Keep server running for a bit to handle requests
    printf("[SERVER] Announcements complete, waiting for requests...\n");
    sleep(10);
    
    // Shutdown
    printf("[SERVER] Shutting down...\n");
    server_running = false;
    
    pthread_join(listener_tid, NULL);
    
    close(udp_sock);
    
    printf("[SERVER] Server stopped\n");
    return 0;
}

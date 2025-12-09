# Minimal DoIP UDP Discovery Implementation

This is a minimal implementation of the DoIP (ISO 13400) UDP discovery mechanism with both server and client.

## Features

- **Server**: Sends periodic Vehicle Announcements and responds to Vehicle Identification Requests
- **Client**: Listens for Vehicle Announcements, stores server IP, and sends Vehicle Identification Request
- **Loopback mode**: For testing client and server on the same host (avoids multicast issues)

## Building

```bash
make
```

This will create two executables:
- `doip_server` - The DoIP server
- `doip_client` - The DoIP client

## Usage

### Running in Loopback Mode (Recommended for Testing)

**Terminal 1 - Start the server:**
```bash
./doip_server --loopback
```

**Terminal 2 - Start the client:**
```bash
./doip_client --loopback
```

### Running in Broadcast Mode (For Network Testing)

**Terminal 1 - Start the server:**
```bash
./doip_server
```

**Terminal 2 - Start the client:**
```bash
./doip_client
```

## How It Works

### Server (Port 13400)
1. Creates two UDP sockets (both bound to port 13400 with SO_REUSEADDR):
   - Receive socket: Listens for incoming Vehicle Identification Requests
   - Send socket: Sends Vehicle Announcements and responses
2. Starts two threads:
   - Listener thread: Continuously listens for requests
   - Announcement thread: Sends 5 Vehicle Announcements (every 2 seconds) to port 13401
3. When a Vehicle Identification Request is received:
   - Parses the request
   - Sends a Vehicle Identification Response back to the client

### Client (Port 13401)
1. Creates two UDP sockets:
   - Request socket: Sends Vehicle Identification Requests (unbound, OS assigns port)
   - Announcement socket: Bound to port 13401 to receive Vehicle Announcements
2. Workflow:
   - Listens for Vehicle Announcement from server
   - Extracts vehicle information (VIN, Logical Address, EID, GID, IP address)
   - Sends Vehicle Identification Request to the discovered server IP on port 13400
   - Waits for and displays the response

## Ports

- **13400**: DoIP UDP Discovery Port (server listens here, client sends requests here)
- **13401**: DoIP Test Equipment Port (client listens here for announcements)

## Protocol Details

### DoIP Header (8 bytes)
- Protocol Version: 0x04
- Inverse Protocol Version: 0xFB
- Payload Type: 2 bytes
- Payload Length: 4 bytes

### Payload Types
- `0x0001`: Vehicle Identification Request (no payload)
- `0x0004`: Vehicle Identification Response (33 bytes minimum)

### Vehicle Identification Response Payload
- VIN: 17 bytes (ASCII)
- Logical Address: 2 bytes
- EID: 6 bytes
- GID: 6 bytes
- Further Action Required: 1 byte
- VIN/GID sync status: 1 byte

## Key Design Decisions

1. **Two sockets on server**: Avoids race conditions between sending announcements and receiving requests
2. **SO_REUSEADDR**: Allows both server sockets to bind to port 13400
3. **Loopback mode**: Uses unicast (127.0.0.1) instead of broadcast for local testing
4. **Non-blocking receive**: Server uses timeout to allow clean shutdown
5. **Separate announcement socket on client**: Dedicated socket for receiving broadcasts on port 13401

## Troubleshooting

If the client doesn't receive announcements:
- Check firewall rules
- Verify both programs are running
- Use `tcpdump` to monitor UDP traffic:
  ```bash
  sudo tcpdump -i any udp port 13400 or udp port 13401 -X
  ```

## Cleanup

```bash
make clean
```

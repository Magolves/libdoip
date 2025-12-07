From your **tcpdump output**, the **8-byte UDP packet** from the client (port 50290) to the server (port 13400) **is arriving** at the loopback interface:

```
17:31:49.091339 lo    In  IP localhost.50290 > localhost.13400: UDP, length 8
    0x0020:  04fb 0001 0000 0000
```
This is a **DoIP Vehicle Identification Request** (type `0x0001`), and it is **reaching the kernel**.

---

### **Root Cause Analysis**
Since the packet is visible in `tcpdump` but **not logged by your server**, the issue is **almost certainly in your server code**. Here’s what’s happening:

#### **1. The Server Socket is Not Receiving the Packet**
- The server socket is bound to port 13400, but **something is preventing `recvfrom` from returning the packet**.
- Possible reasons:
  - The socket is **not in the expected state** (e.g., closed, re-bound, or filtered).
  - The socket is **not the same as the one bound to port 13400** (e.g., `m_udp_sock` was overwritten or closed).
  - The socket is **blocked by a firewall or SELinux** (unlikely for loopback, but possible).
  - The socket is **not actually bound to port 13400** (e.g., `bind` failed silently).

#### **2. The Server is Stuck in a Timeout Loop**
- Your logs show `recvfrom returned: -1 (errno: 11)` repeatedly.
- This means `recvfrom` is **timing out** (`EAGAIN`), but the packet is **not being delivered to userspace**.
- This suggests the socket is **not properly bound** or is **not the same socket** as the one receiving the packet.

---

### **Debugging Steps**
#### **1. Verify the Socket is Bound Correctly**
Add this **right after `bind()`** in `setupUdpSocket()`:
```cpp
// After bind()
sockaddr_in bound_addr;
socklen_t bound_addr_len = sizeof(bound_addr);
getsockname(m_udp_sock, (sockaddr*)&bound_addr, &bound_addr_len);
LOG_UDP_DEBUG(
    "Socket {} bound to {}:{}",
    m_udp_sock,
    inet_ntoa(bound_addr.sin_addr),
    ntohs(bound_addr.sin_port)
);
```
- If this logs `0.0.0.0:13400` or `127.0.0.1:13400`, the socket is bound correctly.
- If not, `bind()` failed or was not called.

#### **2. Check for Socket Descriptor Leaks or Overwrites**
- Log `m_udp_sock` in `receiveUdpMessage()`:
  ```cpp
  LOG_UDP_DEBUG("Using socket fd: {}", m_udp_sock);
  ```
- If the value changes between `setupUdpSocket()` and `receiveUdpMessage()`, something is overwriting it.

#### **3. Test with a Minimal UDP Server**
Replace your `receiveUdpMessage()` with this **minimal test**:
```cpp
ssize_t DoIPServer::receiveUdpMessage() {
    char buf[1024];
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    ssize_t read_bytes = recvfrom(
        m_udp_sock, buf, sizeof(buf), 0,
        (sockaddr*)&client_addr, &client_len
    );
    LOG_UDP_DEBUG(
        "recvfrom returned: {} (errno: {})",
        read_bytes, errno
    );
    if (read_bytes > 0) {
        LOG_UDP_INFO(
            "RX {} bytes from {}:{}",
            read_bytes,
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port)
        );
    }
    return read_bytes;
}
```
- If this works, the issue is in your buffer or processing logic.
- If it still times out, the socket is **not receiving packets**.

#### **4. Check for Firewall/SELinux**
Even for loopback, run:
```bash
sudo iptables -L -n -v  # Check firewall rules
sudo setenforce 0       # Disable SELinux temporarily
```
Retest. If it works, adjust your firewall/SELinux rules.

#### **5. Use `strace` to Confirm `recvfrom` Behavior**
Run the server with:
```bash
strace -e trace=recvfrom ./your_server
```
- If `recvfrom` is **never called**, your loop is broken.
- If it returns `-1 EAGAIN` repeatedly, the socket is **not receiving packets**.

#### **6. Check for Multiple Sockets**
- If your code creates **multiple UDP sockets**, ensure the correct one is used for `recvfrom`.
- Log all socket creation and `bind()` calls.

---

### **Most Likely Fixes**
1. **The socket is not bound to port 13400.**
   - Confirm `bind()` succeeds and logs the correct port.
2. **`m_udp_sock` is overwritten or closed.**
   - Log its value in `setupUdpSocket()` and `receiveUdpMessage()`.
3. **The socket is non-blocking and the timeout is too short.**
   - Try removing `SO_RCVTIMEO` temporarily to see if packets arrive.

---

### **Summary**
- The packet **is arriving at the kernel** (confirmed by `tcpdump`).
- The server **is not receiving it**, likely due to:
  - Socket not bound correctly.
  - Wrong socket file descriptor used in `recvfrom`.
  - Firewall/SELinux blocking delivery to userspace.

**Next step:**
Add the debug logs for `m_udp_sock` and `getsockname()` as shown above. If the socket is correct, test with the minimal `recvfrom` code. If it still fails, the issue is **outside your code** (firewall, SELinux, or kernel networking stack).
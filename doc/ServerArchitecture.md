# DoIP Server Architecture

## Overview

The `DoIPServer` class provides two distinct APIs for different use cases:

### 1. **Low-Level API (Manual Mode)**
For advanced users who need full control over threading and connection management.

### 2. **High-Level API (Automatic Mode)**
For most users who want simple, production-ready server functionality with minimal code.

---

## High-Level API (Recommended)

### Design Philosophy

The high-level API follows these Linux/UNIX best practices:

1. **Encapsulation**: All socket and thread management is internal
2. **Callback-based**: Application logic is provided via std::function callbacks
3. **Resource Management**: RAII principles - destructor ensures cleanup
4. **Thread Safety**: Atomic operations for state management
5. **Graceful Shutdown**: Proper cleanup of all resources on stop()

### Usage Pattern

```cpp
#include "DoIPServer.h"

// Define your connection handler
std::optional<DoIPServerModel> onConnectionAccepted(DoIPConnection* conn) {
    // Create a model for this connection
    DoIPServerModel model;
    model.serverAddress = MY_ADDRESS;

    // Set up callbacks for this connection
    model.onDiagnosticMessage = [](const DoIPMessage &msg) {
        // Handle diagnostic messages
        return std::nullopt; // ACK
    };

    model.onCloseConnection = []() noexcept {
        // Cleanup on disconnect
    };

    return model; // Accept connection
    // or: return std::nullopt; // Reject connection
}

int main() {
    DoIPServer server;

    // Configure server
    server.setVIN("MYVIN12345678901");
    server.setLogicalGatewayAddress(0x0028);

    // Start with automatic handling
    if (!server.start(onConnectionAccepted, true)) {
        return 1;
    }

    // Server is now running in background threads
    // Main thread can do other work or just wait

    waitForShutdownSignal();

    // Graceful shutdown
    server.stop();

    return 0;
}
```

### Threading Model

When `start()` is called, the following threads are created:

1. **UDP Listener Thread**: Continuously processes UDP vehicle identification requests
2. **TCP Acceptor Thread**: Waits for new TCP connections
3. **Connection Handler Threads**: One per active TCP connection (detached)

```
┌─────────────────────────────────────────────┐
│            Main Application Thread           │
│  - Calls server.start()                     │
│  - Does application-specific work           │
│  - Calls server.stop() on shutdown          │
└─────────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
┌───────▼──────┐         ┌────────▼─────────┐
│ UDP Listener │         │ TCP Acceptor     │
│   Thread     │         │    Thread        │
│              │         │                  │
│ - Recv UDP   │         │ - accept()       │
│ - Send VID   │         │ - Invoke         │
│   response   │         │   callback       │
└──────────────┘         │ - Spawn handler  │
                         └────────┬─────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
            ┌───────▼──────┐           ┌────────▼──────┐
            │ Connection   │           │ Connection    │
            │ Handler 1    │    ...    │ Handler N     │
            │              │           │               │
            │ - Recv DoIP  │           │ - Recv DoIP   │
            │ - Call model │           │ - Call model  │
            │   callbacks  │           │   callbacks   │
            └──────────────┘           └───────────────┘
```

### Connection Lifecycle

```
[New TCP Connection]
        │
        ├─> TCP Acceptor accepts connection
        │
        ├─> Creates DoIPConnection object
        │
        ├─> Invokes onConnectionAccepted(connection)
        │       │
        │       ├─> Application returns DoIPServerModel (accept)
        │       │   or std::nullopt (reject)
        │       │
        ├─> If accepted: setServerModel(model)
        │
        ├─> Spawn dedicated connection handler thread
        │       │
        │       ├─> while (active) { receiveTcpMessage() }
        │       │
        │       ├─> Connection closes
        │       │
        │       └─> Thread exits, unique_ptr destroys connection
        │
        └─> TCP Acceptor continues waiting for next connection
```

### Error Handling

The high-level API handles errors gracefully:

- **Socket errors**: Logged and retried (with backoff for accept failures)
- **Connection drops**: Handler thread exits, resources cleaned up
- **Shutdown during operation**: All threads notified via m_running flag
- **Failed start()**: Returns false, all resources cleaned up

### Resource Management

```cpp
DoIPServer::~DoIPServer() {
    if (m_running.load()) {
        stop();  // Automatic cleanup on destruction
    }
}

void DoIPServer::stop() {
    m_running.store(false);  // Signal all threads to stop
    closeUdpSocket();        // Unblock recv calls
    closeTcpSocket();        // Unblock accept calls

    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();   // Wait for clean exit
        }
    }
    // All threads stopped, sockets closed
}
```

---

## Low-Level API (Advanced Users)

### When to Use

Use the low-level API when you need:
- Custom threading models
- Integration with existing event loops
- Fine-grained control over connection acceptance
- Custom timeout handling

### Usage Pattern

```cpp
DoIPServer server;
server.setupTcpSocket();
server.setupUdpSocket();

// You manage the threading
std::thread udpThread([&]() {
    while (running) {
        server.receiveUdpMessage();
    }
});

std::thread tcpThread([&]() {
    server.setupTcpSocket();
    while (running) {
        auto connection = server.waitForTcpConnection();
        if (connection) {
            // You manage the connection lifecycle
            DoIPServerModel model = createModel();
            connection->setServerModel(model);

            while (connection->isSocketActive()) {
                connection->receiveTcpMessage();
            }
        }
    }
});
```

---

## Callback Interface: DoIPServerModel

Each connection is configured with a `DoIPServerModel` that defines its behavior:

```cpp
struct DoIPServerModel {
    // Required: Server's logical address
    DoIPAddress serverAddress;

    // Optional callbacks:
    CloseConnectionHandler onCloseConnection;
    DiagnosticMessageHandler onDiagnosticMessage;
    DiagnosticNotificationHandler onDiagnosticNotification;
};
```

### Callback Signatures

```cpp
// Called when connection closes
using CloseConnectionHandler = std::function<void()>;

// Called when diagnostic message arrives
// Return std::nullopt for ACK, or DoIPNegativeDiagnosticAck for NACK
using DiagnosticMessageHandler =
    std::function<DoIPDiagnosticAck(const DoIPMessage &)>;

// Called after sending diagnostic ACK/NACK
using DiagnosticNotificationHandler =
    std::function<void(DoIPDiagnosticAck)>;
```

---

## Performance Considerations

### Thread Pooling vs Thread-per-Connection

The current implementation uses **thread-per-connection** (detached threads):

**Pros:**
- Simple, clean code
- Excellent for low-to-moderate connection counts (<100)
- No complex synchronization between connections
- Each connection has dedicated CPU time

**Cons:**
- Does not scale to thousands of connections
- Small overhead per thread

**Future Enhancement:**
For high-scale deployments, a thread pool could be added:

```cpp
// Potential future API
server.start(onConnectionAccepted,
             /*sendAnnouncements=*/true,
             /*workerThreads=*/4);  // Thread pool size
```

### Memory Usage

Per active connection:
- DoIPConnection object: ~256 bytes
- Thread stack: 2-8 MB (OS dependent)
- TCP buffers: ~4 KB (receive buffer)

For 10 concurrent connections: ~25 MB

---

## Signal Handling

The high-level API is designed to work with POSIX signals:

```cpp
static std::atomic<bool> g_shutdown{false};

void signalHandler(int sig) {
    g_shutdown.store(true);
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    server.start(onConnectionAccepted);

    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(100ms);
    }

    server.stop();  // Clean shutdown
}
```

---

## Comparison with Other Patterns

### vs. libevent/libev (Event Loop)

**Event Loop:**
```cpp
// Pseudo-code
while (running) {
    events = epoll_wait(epollfd, ...);
    for (event : events) {
        handle(event);
    }
}
```

**DoIPServer High-Level API:**
```cpp
server.start(onConnectionAccepted);
// That's it!
```

**Trade-off:** Event loops are more scalable but require significant boilerplate. DoIPServer prioritizes simplicity for typical automotive use cases (<100 connections).

### vs. Boost.Asio (Async I/O)

**Boost.Asio:**
```cpp
async_accept(socket, [](error_code ec, socket s) {
    async_read(s, buffer, [](error_code ec, size_t n) {
        // nested callbacks...
    });
});
io_context.run();
```

**DoIPServer:**
```cpp
model.onDiagnosticMessage = [](const DoIPMessage &msg) {
    // synchronous, clean callback
};
server.start(onConnectionAccepted);
```

**Trade-off:** Asio is more powerful but complex. DoIPServer uses synchronous I/O with threads for simplicity.

---

## Best Practices

1. **Always call stop() before destruction** (or rely on RAII)
2. **Keep callbacks fast** - they run on connection threads
3. **Use atomic/mutex for shared state** accessed from callbacks
4. **Return std::nullopt from onConnectionAccepted** to rate-limit connections
5. **Set reasonable timeouts** in socket operations
6. **Handle exceptions in callbacks** - they're on background threads

---

## Examples

See:
- `examples/exampleDoIPServerSimple.cpp` - High-level API usage
- `examples/exampleDoIPServer.cpp` - Low-level API usage (manual mode)

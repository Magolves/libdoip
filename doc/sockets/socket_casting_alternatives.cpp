#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <span>
#include <array>
#include <type_traits>
#include <iostream>

// ============================================================================
// Problem: Typische Socket-API Verwendung mit reinterpret_cast
// ============================================================================

namespace unsafe_example {

void typical_socket_code() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(13400);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    // ❌ Problematisch: reinterpret_cast
    bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    
    // ❌ Bei send/recv auch problematisch
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    send(sockfd, reinterpret_cast<const char*>(data.data()), data.size(), 0);
}

} // namespace unsafe_example

// ============================================================================
// Lösung 1: std::bit_cast (C++20) für Type-Punning
// ============================================================================

namespace solution1_bitcast {

#if __cplusplus >= 202002L
#include <bit>

// std::bit_cast ist der sicherste Weg für Type-Punning
// Aber funktioniert nur für gleich große Typen

// Für kleine Strukturen OK:
void example_bitcast() {
    struct A { int x; };
    struct B { int y; };
    
    A a{42};
    B b = std::bit_cast<B>(a);  // ✅ Sicher
}
#endif

} // namespace solution1_bitcast

// ============================================================================
// Lösung 2: Wrapper-Klassen mit Union (für sockaddr)
// ============================================================================

namespace solution2_union {

/**
 * @brief Type-safe socket address wrapper
 * 
 * Verwendet union für legales Type-Punning (seit C++11)
 */
class SocketAddress {
public:
    SocketAddress() {
        std::memset(&storage_, 0, sizeof(storage_));
    }
    
    // IPv4
    static SocketAddress createIPv4(const char* ip, uint16_t port) {
        SocketAddress addr;
        auto& ipv4 = addr.storage_.ipv4;
        ipv4.sin_family = AF_INET;
        ipv4.sin_port = htons(port);
        inet_pton(AF_INET, ip, &ipv4.sin_addr);
        addr.length_ = sizeof(sockaddr_in);
        return addr;
    }
    
    // IPv6
    static SocketAddress createIPv6(const char* ip, uint16_t port) {
        SocketAddress addr;
        auto& ipv6 = addr.storage_.ipv6;
        ipv6.sin6_family = AF_INET6;
        ipv6.sin6_port = htons(port);
        inet_pton(AF_INET6, ip, &ipv6.sin6_addr);
        addr.length_ = sizeof(sockaddr_in6);
        return addr;
    }
    
    // ✅ Sichere Konvertierung zu sockaddr*
    sockaddr* get() { return &storage_.base; }
    const sockaddr* get() const { return &storage_.base; }
    
    socklen_t length() const { return length_; }
    socklen_t* length_ptr() { return &length_; }
    
    // Typsicherer Zugriff
    bool isIPv4() const { return storage_.base.sa_family == AF_INET; }
    bool isIPv6() const { return storage_.base.sa_family == AF_INET6; }
    
    sockaddr_in& asIPv4() { return storage_.ipv4; }
    const sockaddr_in& asIPv4() const { return storage_.ipv4; }
    
    sockaddr_in6& asIPv6() { return storage_.ipv6; }
    const sockaddr_in6& asIPv6() const { return storage_.ipv6; }
    
private:
    union Storage {
        sockaddr base;
        sockaddr_in ipv4;
        sockaddr_in6 ipv6;
        sockaddr_storage storage;
    } storage_;
    
    socklen_t length_ = 0;
};

void example_usage() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // ✅ Typsicher ohne reinterpret_cast
    auto addr = SocketAddress::createIPv4("127.0.0.1", 13400);
    bind(sockfd, addr.get(), addr.length());
    
    // ✅ Auch bei accept
    SocketAddress client_addr;
    int client_fd = accept(sockfd, client_addr.get(), client_addr.length_ptr());
    
    if (client_addr.isIPv4()) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.asIPv4().sin_addr, ip, sizeof(ip));
        std::cout << "IPv4 client: " << ip << std::endl;
    }
}

} // namespace solution2_union

// ============================================================================
// Lösung 3: std::span für send/recv (C++20)
// ============================================================================

namespace solution3_span {

/**
 * @brief Type-safe socket send/recv wrapper
 */
class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}
    
    // ✅ Send mit std::span - kein cast nötig
    template<typename T>
    ssize_t send(std::span<const T> data, int flags = 0) {
        static_assert(std::is_trivially_copyable_v<T>, 
                     "Type must be trivially copyable for socket transmission");
        
        // std::span gibt uns bereits void* kompatible Daten
        auto bytes = std::as_bytes(data);
        return ::send(fd_, bytes.data(), bytes.size(), flags);
    }
    
    // ✅ Recv mit std::span
    template<typename T>
    ssize_t recv(std::span<T> buffer, int flags = 0) {
        static_assert(std::is_trivially_copyable_v<T>,
                     "Type must be trivially copyable for socket transmission");
        
        auto bytes = std::as_writable_bytes(buffer);
        return ::recv(fd_, bytes.data(), bytes.size(), flags);
    }
    
    // Überladungen für ByteArray/vector
    ssize_t send(const std::vector<uint8_t>& data, int flags = 0) {
        return send(std::span{data}, flags);
    }
    
    ssize_t recv(std::vector<uint8_t>& buffer, int flags = 0) {
        return recv(std::span{buffer}, flags);
    }
    
private:
    int fd_;
};

void example_usage() {
    Socket sock(/* fd */);
    
    // ✅ Kein reinterpret_cast!
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    sock.send(data);
    
    // ✅ Auch mit Arrays
    std::array<uint8_t, 1024> buffer;
    sock.recv(std::span{buffer});
    
    // ✅ Oder mit rohen Pointern
    uint8_t raw_buffer[512];
    sock.recv(std::span{raw_buffer});
}

} // namespace solution3_span

// ============================================================================
// Lösung 4: std::byte für Byte-Buffer (C++17)
// ============================================================================

namespace solution4_byte {

/**
 * @brief Socket wrapper mit std::byte
 */
class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}
    
    // ✅ std::byte ist semantisch korrekter als uint8_t für Bytes
    ssize_t send(const std::byte* data, size_t size, int flags = 0) {
        return ::send(fd_, data, size, flags);
    }
    
    ssize_t recv(std::byte* data, size_t size, int flags = 0) {
        return ::recv(fd_, data, size, flags);
    }
    
    // Convenience für vector<std::byte>
    ssize_t send(const std::vector<std::byte>& data, int flags = 0) {
        return send(data.data(), data.size(), flags);
    }
    
    ssize_t recv(std::vector<std::byte>& data, int flags = 0) {
        return recv(data.data(), data.size(), flags);
    }
    
private:
    int fd_;
};

void example_usage() {
    Socket sock(/* fd */);
    
    // ✅ std::byte ist typsicherer für Rohdaten
    std::vector<std::byte> data = {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}
    };
    sock.send(data);
    
    // Konvertierung von/zu uint8_t wenn nötig
    std::vector<uint8_t> uint8_data = {0x01, 0x02, 0x03};
    auto byte_data = std::as_bytes(std::span{uint8_data});
    sock.send(reinterpret_cast<const std::byte*>(byte_data.data()), 
              byte_data.size());
}

} // namespace solution4_byte

// ============================================================================
// Lösung 5: Memcpy statt reinterpret_cast (für alte Compiler)
// ============================================================================

namespace solution5_memcpy {

/**
 * @brief Sichere Alternative für C++11/14/17
 * 
 * memcpy ist vom Standard garantiert korrekt für Type-Punning
 */
class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}
    
    // ✅ memcpy ist sicherer als reinterpret_cast
    template<typename T>
    ssize_t send(const T* data, size_t count, int flags = 0) {
        static_assert(std::is_trivially_copyable<T>::value,
                     "Type must be trivially copyable");
        
        // memcpy ist vom Standard erlaubt für Type-Punning
        std::vector<char> buffer(count * sizeof(T));
        std::memcpy(buffer.data(), data, buffer.size());
        
        return ::send(fd_, buffer.data(), buffer.size(), flags);
    }
    
    // Aber für vector können wir direkten Zugriff nutzen
    ssize_t sendBytes(const std::vector<uint8_t>& data, int flags = 0) {
        // ✅ Der Cast von uint8_t* zu char* ist erlaubt
        return ::send(fd_, 
                     reinterpret_cast<const char*>(data.data()), 
                     data.size(), 
                     flags);
    }
};

} // namespace solution5_memcpy

// ============================================================================
// Lösung 6: Kombinierter moderner Socket-Wrapper
// ============================================================================

namespace solution6_complete {

/**
 * @brief Moderne, typsichere Socket-Klasse
 * 
 * Kombiniert alle Best Practices
 */
class Socket {
public:
    Socket() : fd_(-1) {}
    explicit Socket(int fd) : fd_(fd) {}
    
    // RAII: Automatisches Schließen
    ~Socket() { close(); }
    
    // Move-only (kein Kopieren von File Descriptors)
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    Socket(Socket&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }
    
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
    
    // Factory-Methoden
    static Socket createTCP() {
        return Socket(::socket(AF_INET, SOCK_STREAM, 0));
    }
    
    static Socket createUDP() {
        return Socket(::socket(AF_INET, SOCK_DGRAM, 0));
    }
    
    // Bind mit SocketAddress
    bool bind(const solution2_union::SocketAddress& addr) {
        return ::bind(fd_, addr.get(), addr.length()) == 0;
    }
    
    // Connect mit SocketAddress
    bool connect(const solution2_union::SocketAddress& addr) {
        return ::connect(fd_, addr.get(), addr.length()) == 0;
    }
    
    // Listen
    bool listen(int backlog = 5) {
        return ::listen(fd_, backlog) == 0;
    }
    
    // Accept
    Socket accept(solution2_union::SocketAddress* addr = nullptr) {
        if (addr) {
            return Socket(::accept(fd_, addr->get(), addr->length_ptr()));
        } else {
            return Socket(::accept(fd_, nullptr, nullptr));
        }
    }
    
    // ✅ Type-safe send mit std::span (C++20)
#if __cplusplus >= 202002L
    template<typename T>
    ssize_t send(std::span<const T> data, int flags = 0) {
        static_assert(std::is_trivially_copyable_v<T>);
        auto bytes = std::as_bytes(data);
        return ::send(fd_, bytes.data(), bytes.size(), flags);
    }
    
    template<typename T>
    ssize_t recv(std::span<T> buffer, int flags = 0) {
        static_assert(std::is_trivially_copyable_v<T>);
        auto bytes = std::as_writable_bytes(buffer);
        return ::recv(fd_, bytes.data(), bytes.size(), flags);
    }
#endif
    
    // ✅ Send/recv für vector (C++11 kompatibel)
    ssize_t send(const std::vector<uint8_t>& data, int flags = 0) {
        return ::send(fd_, 
                     reinterpret_cast<const char*>(data.data()), 
                     data.size(), 
                     flags);
    }
    
    ssize_t recv(std::vector<uint8_t>& buffer, int flags = 0) {
        return ::recv(fd_,
                     reinterpret_cast<char*>(buffer.data()),
                     buffer.size(),
                     flags);
    }
    
    // ✅ Raw pointer interface
    ssize_t send(const void* data, size_t size, int flags = 0) {
        return ::send(fd_, data, size, flags);
    }
    
    ssize_t recv(void* buffer, size_t size, int flags = 0) {
        return ::recv(fd_, buffer, size, flags);
    }
    
    // Getter
    int fd() const { return fd_; }
    bool isValid() const { return fd_ >= 0; }
    
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
private:
    int fd_;
};

// ============================================================================
// Verwendungsbeispiel
// ============================================================================

void example_server() {
    using namespace solution2_union;
    
    // ✅ Kein reinterpret_cast in Sicht!
    auto server_socket = Socket::createTCP();
    auto addr = SocketAddress::createIPv4("0.0.0.0", 13400);
    
    if (!server_socket.bind(addr)) {
        std::cerr << "Bind failed\n";
        return;
    }
    
    if (!server_socket.listen()) {
        std::cerr << "Listen failed\n";
        return;
    }
    
    std::cout << "Listening on port 13400...\n";
    
    SocketAddress client_addr;
    auto client_socket = server_socket.accept(&client_addr);
    
    if (client_socket.isValid()) {
        std::cout << "Client connected\n";
        
        // ✅ Type-safe senden
        std::vector<uint8_t> response = {0x01, 0x02, 0x03, 0x04};
        client_socket.send(response);
        
        // ✅ Type-safe empfangen
        std::vector<uint8_t> buffer(1024);
        ssize_t n = client_socket.recv(buffer);
        if (n > 0) {
            buffer.resize(n);
            std::cout << "Received " << n << " bytes\n";
        }
    }
}

void example_client() {
    using namespace solution2_union;
    
    auto socket = Socket::createTCP();
    auto addr = SocketAddress::createIPv4("127.0.0.1", 13400);
    
    if (socket.connect(addr)) {
        std::cout << "Connected!\n";
        
        // ✅ Kein reinterpret_cast
        std::vector<uint8_t> request = {0x10, 0x01};
        socket.send(request);
        
        std::vector<uint8_t> response(1024);
        ssize_t n = socket.recv(response);
        response.resize(n);
    }
}

} // namespace solution6_complete

// ============================================================================
// Zusammenfassung und Best Practices
// ============================================================================

int main() {
    std::cout << "Socket Casting Best Practices:\n\n";
    
    std::cout << "1. SocketAddress Wrapper (union-based)\n";
    std::cout << "   ✅ Typsicher für bind/connect/accept\n";
    std::cout << "   ✅ Keine reinterpret_casts\n";
    std::cout << "   ✅ C++11 kompatibel\n\n";
    
    std::cout << "2. std::span für send/recv (C++20)\n";
    std::cout << "   ✅ Generisch und typsicher\n";
    std::cout << "   ✅ Compile-time checks\n";
    std::cout << "   ✅ std::as_bytes() eliminiert Casts\n\n";
    
    std::cout << "3. std::byte für Bytebuffer (C++17)\n";
    std::cout << "   ✅ Semantisch korrekter als uint8_t\n";
    std::cout << "   ✅ Verhindert arithmetische Operationen\n\n";
    
    std::cout << "4. RAII Socket Wrapper\n";
    std::cout << "   ✅ Automatisches close()\n";
    std::cout << "   ✅ Move-only Semantik\n";
    std::cout << "   ✅ Saubere API\n\n";
    
    std::cout << "5. uint8_t* zu char* Cast\n";
    std::cout << "   ✅ Dieser Cast ist vom Standard erlaubt\n";
    std::cout << "   ✅ Für Legacy-Kompatibilität OK\n\n";
    
    return 0;
}

} // namespace solution6_complete

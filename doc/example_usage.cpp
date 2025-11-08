#include "DoIPMessage_improved.h"
#include <iostream>
#include <cstring>

// Simulierte Socket-Klasse für Beispiele
class Socket {
public:
    void send(const uint8_t* data, size_t size) {
        std::cout << "Sending " << size << " bytes: ";
        for (size_t i = 0; i < std::min(size, size_t(16)); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<unsigned>(data[i]) << " ";
        }
        if (size > 16) std::cout << "...";
        std::cout << std::dec << std::endl;
    }
};

void example1_basic_usage() {
    std::cout << "\n=== Beispiel 1: Grundlegende Verwendung ===\n";
    
    using namespace doip;
    
    // Nachrichten erstellen
    auto vehicleIdReq = message::makeVehicleIdentificationRequest();
    auto aliveCheckReq = message::makeAliveCheckRequest();
    
    std::cout << "Vehicle ID Request: " << vehicleIdReq << std::endl;
    std::cout << "Alive Check Request: " << aliveCheckReq << std::endl;
    
    // Größeninfo
    std::cout << "Message size: " << vehicleIdReq.getMessageSize() << " bytes\n";
    std::cout << "Payload size: " << vehicleIdReq.getPayloadSize() << " bytes\n";
}

void example2_zero_copy_sending() {
    std::cout << "\n=== Beispiel 2: Zero-Copy Senden ===\n";
    
    using namespace doip;
    
    // Diagnostic-Nachricht erstellen
    DoIPAddress sourceAddr(0x0E80);
    DoIPAddress targetAddr(0x1234);
    ByteArray udsPayload = {0x10, 0x01}; // DiagnosticSessionControl
    
    auto diagMsg = message::makeDiagnosticMessage(sourceAddr, targetAddr, udsPayload);
    
    std::cout << "Diagnostic Message: " << diagMsg << std::endl;
    
    Socket socket;
    
    // Methode 1: Direkt mit data() und size()
    std::cout << "\nMethode 1 (Legacy-kompatibel):\n";
    socket.send(diagMsg.data(), diagMsg.size());
    
    // Methode 2: Mit ByteArrayRef
    std::cout << "\nMethode 2 (ByteArrayRef):\n";
    auto data = diagMsg.getData();
    socket.send(data.first, data.second);
    
    // ❌ NICHT MEHR NÖTIG (alte Methode):
    // ByteArray bytes = diagMsg.toByteArray();  // würde Kopie erstellen
    // socket.send(bytes.data(), bytes.size());
}

void example3_parsing_received_messages() {
    std::cout << "\n=== Beispiel 3: Empfangene Nachrichten parsen ===\n";
    
    using namespace doip;
    
    // Simuliere empfangene Daten
    uint8_t receivedBuffer[] = {
        0x04,       // Protocol version
        0xFB,       // Inverse protocol version
        0x80, 0x01, // Payload type (DiagnosticMessage = 0x8001)
        0x00, 0x00, 0x00, 0x05, // Payload length (5 bytes)
        0x0E, 0x80, // Source address
        0x12, 0x34, // Target address
        0x50        // UDS response
    };
    
    // Nachricht parsen
    auto msg = DoIPMessage::parse(receivedBuffer, sizeof(receivedBuffer));
    
    if (msg && msg->isValid()) {
        std::cout << "Empfangene Nachricht: " << *msg << std::endl;
        
        // Payload-Typ prüfen
        if (msg->getPayloadType() == DoIPPayloadType::DiagnosticMessage) {
            auto sa = msg->getSourceAddress();
            auto ta = msg->getTargetAddress();
            
            if (sa && ta) {
                std::cout << "Source Address: 0x" << std::hex 
                         << sa->value() << std::dec << std::endl;
                std::cout << "Target Address: 0x" << std::hex 
                         << ta->value() << std::dec << std::endl;
            }
            
            // Zugriff auf Payload
            auto payload = msg->getPayload();
            std::cout << "Payload: ";
            for (size_t i = 0; i < payload.second; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                         << static_cast<unsigned>(payload.first[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
    } else {
        std::cout << "Ungültige Nachricht empfangen!" << std::endl;
    }
}

void example4_different_message_types() {
    std::cout << "\n=== Beispiel 4: Verschiedene Nachrichtentypen ===\n";
    
    using namespace doip;
    
    // 1. Negative ACK
    auto nackMsg = message::makeNegativeAckMessage(DoIPNegativeAck::InvalidPayloadLength);
    std::cout << "NACK Message: " << nackMsg << std::endl;
    
    // 2. Alive Check Response
    DoIPAddress entityAddr(0x0E80);
    auto aliveResp = message::makeAliveCheckResponse(entityAddr);
    std::cout << "Alive Check Response: " << aliveResp << std::endl;
    
    // 3. Routing Activation Request
    auto routingReq = message::makeRoutingRequest(entityAddr, DoIPActivationType::Default);
    std::cout << "Routing Request: " << routingReq << std::endl;
    
    // 4. Diagnostic Positive Response
    DoIPAddress sourceAddr(0x1234);
    DoIPAddress targetAddr(0x0E80);
    ByteArray udsResponse = {0x50, 0x01}; // Positive response to 0x10 0x01
    
    auto diagPosResp = message::makeDiagnosticPositiveResponse(
        sourceAddr, targetAddr, udsResponse
    );
    std::cout << "Diagnostic Positive Response: " << diagPosResp << std::endl;
    
    // 5. Diagnostic Negative Response
    auto diagNegResp = message::makeDiagnosticNegativeResponse(
        sourceAddr, targetAddr, 
        DoIPNegativeDiagnosticAck::TargetUnreachable,
        udsResponse
    );
    std::cout << "Diagnostic Negative Response: " << diagNegResp << std::endl;
}

void example5_memory_comparison() {
    std::cout << "\n=== Beispiel 5: Memory-Vergleich (Alt vs. Neu) ===\n";
    
    using namespace doip;
    
    // Große Diagnostic-Nachricht erstellen (z.B. 4KB Payload)
    DoIPAddress sourceAddr(0x0E80);
    DoIPAddress targetAddr(0x1234);
    ByteArray largePayload(4096, 0xAA); // 4KB mit 0xAA gefüllt
    
    auto msg = message::makeDiagnosticMessage(sourceAddr, targetAddr, largePayload);
    
    std::cout << "Nachrichtengröße: " << msg.getMessageSize() << " bytes\n";
    
    // ALTE Methode (würde Kopie erstellen):
    // ByteArray copyBuffer = msg.toByteArray();  // ❌ 4100 Bytes kopiert!
    std::cout << "\n❌ Alte Methode:\n";
    std::cout << "   - toByteArray() würde " << msg.getMessageSize() 
              << " bytes kopieren\n";
    std::cout << "   - Memory-Overhead: 2x Nachrichtengröße im RAM\n";
    std::cout << "   - Performance: O(n) Kopierzeit\n";
    
    // NEUE Methode (Zero-Copy):
    auto data = msg.getData();
    std::cout << "\n✅ Neue Methode:\n";
    std::cout << "   - getData() liefert nur Pointer + Größe\n";
    std::cout << "   - Memory-Overhead: nur sizeof(pointer) + sizeof(size_t) ≈ 16 bytes\n";
    std::cout << "   - Performance: O(1) - keine Kopie!\n";
    
    std::cout << "\nErsparnis bei 4KB Nachricht:\n";
    std::cout << "   - Memory: 4100 bytes\n";
    std::cout << "   - Zeit: ~" << msg.getMessageSize() << " CPU-Zyklen\n";
}

void example6_move_semantics() {
    std::cout << "\n=== Beispiel 6: Move-Semantics ===\n";
    
    using namespace doip;
    
    // Payload erstellen
    ByteArray payload = {0x10, 0x01, 0x02, 0x03, 0x04};
    
    std::cout << "Payload vor Move: " << payload.size() << " bytes\n";
    
    // Mit Move-Semantik (effizient)
    auto msg = DoIPMessage(DoIPPayloadType::DiagnosticMessage, std::move(payload));
    
    std::cout << "Payload nach Move: " << payload.size() << " bytes (moved!)\n";
    std::cout << "Message: " << msg << std::endl;
    
    // Die ursprüngliche payload ist jetzt leer (moved from)
    // aber die Daten sind in msg.m_data ohne Kopie!
}

void example7_factory_pattern() {
    std::cout << "\n=== Beispiel 7: Factory-Pattern Vorteile ===\n";
    
    using namespace doip;
    
    // Übersichtlicher Namespace statt langer Klassennamen
    std::cout << "Vorher: DoIPMessage::makeDiagnosticMessage(...)\n";
    std::cout << "Nachher: doip::message::makeDiagnosticMessage(...)\n\n";
    
    // Gruppierung nach Funktionalität möglich
    // Alle Factory-Funktionen sind in doip::message::
    // Zukünftige Erweiterungen können in Unter-Namespaces:
    // - doip::message::diagnostic::
    // - doip::message::routing::
    // - doip::message::vehicle::
    
    std::cout << "Verfügbare Message-Factories:\n";
    std::cout << "  - makeVehicleIdentificationRequest()\n";
    std::cout << "  - makeVehicleIdentificationResponse(...)\n";
    std::cout << "  - makeNegativeAckMessage(...)\n";
    std::cout << "  - makeDiagnosticMessage(...)\n";
    std::cout << "  - makeDiagnosticPositiveResponse(...)\n";
    std::cout << "  - makeDiagnosticNegativeResponse(...)\n";
    std::cout << "  - makeAliveCheckRequest()\n";
    std::cout << "  - makeAliveCheckResponse(...)\n";
    std::cout << "  - makeRoutingRequest(...)\n";
    std::cout << "  - makeRoutingResponse(...)\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  DoIPMessage - Improved Implementation Examples  ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    example1_basic_usage();
    example2_zero_copy_sending();
    example3_parsing_received_messages();
    example4_different_message_types();
    example5_memory_comparison();
    example6_move_semantics();
    example7_factory_pattern();
    
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║              Zusammenfassung der Vorteile         ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    std::cout << "✅ Zero-Copy beim Senden (keine toByteArray() mehr)\n";
    std::cout << "✅ Bessere Code-Organisation (Namespace-Trennung)\n";
    std::cout << "✅ Effizientere Memory-Nutzung\n";
    std::cout << "✅ Move-Semantics optimiert\n";
    std::cout << "✅ Klarere API und bessere Lesbarkeit\n";
    std::cout << "✅ Type-Safe und validierbar\n";
    
    return 0;
}

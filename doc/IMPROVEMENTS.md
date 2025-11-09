# DoIPMessage Verbesserungen

## Übersicht der Änderungen

Diese überarbeitete Version der `DoIPMessage`-Klasse löst die beiden Hauptprobleme:

1. **Eliminierung von Kopien**: Die komplette Nachricht (Header + Payload) wird intern als `ByteArray` gespeichert
2. **Verbesserte Lesbarkeit**: Factory-Funktionen in separatem Namespace statt alles in einer Klasse

## Hauptverbesserungen

### 1. Interne ByteArray-Repräsentation

**Vorher:**
```cpp
struct DoIPMessage {
    DoIPPayloadType m_payload_type;
    DoIPPayload m_payload;
    
    ByteArray toByteArray() const {
        ByteArray bytes;
        bytes.reserve(getMessageSize());
        bytes.emplace_back(PROTOCOL_VERSION);
        bytes.emplace_back(PROTOCOL_VERSION_INV);
        appendPayloadType(bytes);
        appendSize(bytes);
        bytes.insert(bytes.end(), m_payload.begin(), m_payload.end());
        return bytes;  // ❌ Kopie!
    }
};
```

**Nachher:**
```cpp
class DoIPMessage {
protected:
    ByteArray m_data; // Komplette Nachricht inkl. Header
    
public:
    ByteArrayRef getData() const {
        return {m_data.data(), m_data.size()};  // ✅ Keine Kopie!
    }
    
    const uint8_t* data() const { return m_data.data(); }
    size_t size() const { return m_data.size(); }
};
```

**Memory Layout:**
```
m_data = [V][~V][Type_H][Type_L][Size3][Size2][Size1][Size0][Payload...]
         [0] [1]  [2]     [3]     [4]    [5]    [6]    [7]    [8...]
         |<-------- Header (8 Bytes) -------->|<----- Payload ----->|
```

### 2. Factory-Funktionen im separaten Namespace

**Vorher:**
```cpp
struct DoIPMessage {
    static DoIPMessage makeVehicleIdentificationRequest() { ... }
    static DoIPMessage makeVehicleIdentificationResponse(...) { ... }
    static DoIPMessage makeNegativeAckMessage(...) { ... }
    static DoIPMessage makeDiagnosticMessage(...) { ... }
    static DoIPMessage makeDiagnosticPositiveResponse(...) { ... }
    // ... viele weitere statische Methoden
};
```

**Nachher:**
```cpp
namespace message {
    inline DoIPMessage makeVehicleIdentificationRequest() { ... }
    inline DoIPMessage makeVehicleIdentificationResponse(...) { ... }
    inline DoIPMessage makeNegativeAckMessage(...) { ... }
    inline DoIPMessage makeDiagnosticMessage(...) { ... }
    inline DoIPMessage makeDiagnosticPositiveResponse(...) { ... }
}
```

### 3. Verbesserte API für Zero-Copy Sending

**Verwendung beim Senden:**

```cpp
// Nachricht erstellen
auto msg = doip::message::makeDiagnosticMessage(sourceAddr, targetAddr, udsData);

// Direkt senden ohne Kopie (Variante 1: ByteArrayRef)
auto data = msg.getData();
socket.send(data.first, data.second);

// Oder mit direktem Pointer (Variante 2: für Legacy-APIs)
socket.send(msg.data(), msg.size());

// ❌ NICHT MEHR NÖTIG:
// ByteArray bytes = msg.toByteArray();  // Kopie!
// socket.send(bytes.data(), bytes.size());
```

### 4. Parsing von empfangenen Nachrichten

```cpp
// Nachricht aus Netzwerk-Buffer parsen
std::optional<DoIPMessage> msg = DoIPMessage::parse(buffer, bufferSize);

if (msg) {
    // Validierung
    if (msg->isValid()) {
        // Zugriff auf Daten ohne weitere Kopien
        auto payloadType = msg->getPayloadType();
        auto payload = msg->getPayload();  // ByteArrayRef
        
        // Weiterverarbeitung
        if (payloadType == DoIPPayloadType::DiagnosticMessage) {
            auto sourceAddr = msg->getSourceAddress();
            auto targetAddr = msg->getTargetAddress();
            // ...
        }
    }
}
```

## API-Vergleich

### Nachrichtenerstellung

**Alt:**
```cpp
auto msg = DoIPMessage::makeDiagnosticMessage(sa, ta, payload);
```

**Neu:**
```cpp
auto msg = doip::message::makeDiagnosticMessage(sa, ta, payload);
```

### Senden

**Alt:**
```cpp
ByteArray bytes = msg.toByteArray();  // ❌ Kopie
socket.send(bytes.data(), bytes.size());
```

**Neu:**
```cpp
socket.send(msg.data(), msg.size());  // ✅ Keine Kopie
```

### Payload-Zugriff

**Alt:**
```cpp
const DoIPPayload& payload = msg.getPayload();  // std::vector<uint8_t>
```

**Neu:**
```cpp
ByteArrayRef payload = msg.getPayload();  // std::pair<const uint8_t*, size_t>
// Zugriff: payload.first[i] oder std::span in C++20
```

## Performance-Vorteile

1. **Keine Kopie beim Senden**: Direkter Zugriff auf internen Buffer
2. **Bessere Cache-Lokalität**: Alles in einem zusammenhängenden Speicherbereich
3. **Move-Semantik optimiert**: Nur Pointer-Übergabe statt Daten-Kopie

## Migration von bestehendem Code

### Schritt 1: Namespace-Änderungen
```cpp
// Vorher
auto msg = DoIPMessage::makeDiagnosticMessage(sa, ta, data);

// Nachher
auto msg = doip::message::makeDiagnosticMessage(sa, ta, data);
```

### Schritt 2: toByteArray() entfernen
```cpp
// Vorher
ByteArray bytes = msg.toByteArray();
socket.send(bytes.data(), bytes.size());

// Nachher
socket.send(msg.data(), msg.size());
```

### Schritt 3: Payload-Zugriff anpassen
```cpp
// Vorher
const DoIPPayload& payload = msg.getPayload();
for (auto byte : payload) { ... }

// Nachher
auto payload = msg.getPayload();
for (size_t i = 0; i < payload.second; ++i) {
    uint8_t byte = payload.first[i];
    // ...
}

// Oder mit C++20 std::span:
std::span<const uint8_t> payloadSpan(payload.first, payload.second);
for (auto byte : payloadSpan) { ... }
```

## Weitere Verbesserungen

### 1. Type Safety
Die Klasse ist jetzt eine echte Klasse (nicht mehr struct), mit private Members und kontrolliertem Zugriff.

### 2. Validierung
```cpp
if (msg.isValid()) {
    // Nachricht hat korrekte Struktur
}
```

### 3. Konsistente Payload-Länge
Die Payload-Länge im Header wird automatisch mit der tatsächlichen Größe synchronisiert.

## Optionale Erweiterungen

### C++20: std::span Support

```cpp
// In DoIPMessage.h nach #include <optional>
#if __cplusplus >= 202002L
#include <span>

std::span<const uint8_t> getPayloadSpan() const {
    if (m_data.size() <= DOIP_HEADER_SIZE) {
        return {};
    }
    return std::span<const uint8_t>(
        m_data.data() + DOIP_HEADER_SIZE, 
        m_data.size() - DOIP_HEADER_SIZE
    );
}
#endif
```

### Erweiterte Nachrichtentypen

Für komplexere Nachrichten könnten spezialisierte Klassen erstellt werden:

```cpp
class DiagnosticMessage : public DoIPMessage {
public:
    DiagnosticMessage(const DoIPAddress& sa, const DoIPAddress& ta, 
                     const ByteArray& payload) 
        : DoIPMessage(DoIPPayloadType::DiagnosticMessage, buildPayload(sa, ta, payload)) {}
    
    DoIPAddress getSourceAddress() const;
    DoIPAddress getTargetAddress() const;
    ByteArrayRef getDiagnosticPayload() const;
    
private:
    static ByteArray buildPayload(const DoIPAddress& sa, const DoIPAddress& ta, 
                                  const ByteArray& payload);
};
```

## Zusammenfassung

**Hauptvorteile:**
- ✅ Keine unnötigen Kopien beim Senden
- ✅ Bessere Code-Organisation durch Namespace-Trennung
- ✅ Effizientere Memory-Nutzung
- ✅ Gleiche oder bessere Performance
- ✅ Sauberere API

**Migration:**
- Relativ einfach durchzuführen
- Hauptsächlich Namespace-Änderungen
- `toByteArray()` durch direkten Zugriff ersetzen
- Wenige Änderungen bei Payload-Zugriff

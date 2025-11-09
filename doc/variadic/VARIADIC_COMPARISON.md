# Variadic Template Approach für DoIPMessage

## Ist das verrückt oder machbar? → **Definitiv machbar und sehr elegant!**

## Konzept

Die Idee ist, DoIP-Nachrichten als **typsichere, compile-time validierte Strukturen** zu definieren:

```cpp
// Nachricht wird durch ihre Felder definiert
using RoutingActivationRequest = DoIPMessage<
    DoIPPayloadType::RoutingActivationRequest,  // Payload-Typ
    DoIPAddress,                                 // Feld 1: Source Address
    DoIPByteField<DoIPActivationType>,          // Feld 2: Activation Type
    DoIPReserved<4>                             // Feld 3: 4 Reserved Bytes
>;

// Verwendung
DoIPAddress addr(0x0E80);
DoIPByteField<DoIPActivationType> actType(DoIPActivationType::Default);
DoIPReserved<4> reserved;

RoutingActivationRequest msg(addr, actType, reserved);
```

## Vorteile

### 1. **Compile-Time Type Safety** ✅

```cpp
// ✅ Korrekt
RoutingActivationRequest msg(
    DoIPAddress(0x0E80),
    DoIPByteField<DoIPActivationType>(DoIPActivationType::Default),
    DoIPReserved<4>()
);

// ❌ Compile-Fehler: Falscher Typ
RoutingActivationRequest invalid(
    DoIPVIN("test"),  // Falscher Typ für dieses Feld!
    // ...
);

// ❌ Compile-Fehler: Falsche Anzahl von Argumenten
RoutingActivationRequest invalid2(DoIPAddress(0x0E80));
```

### 2. **Deklarativer, selbstdokumentierender Code** ✅

```cpp
// Die Message-Definition IST die Dokumentation!
using VehicleIdentificationResponse = DoIPMessage<
    DoIPPayloadType::VehicleIdentificationResponse,
    DoIPVIN,           // 17 bytes - VIN
    DoIPAddress,       // 2 bytes - Logical Address
    DoIPEID,           // 6 bytes - Entity ID
    DoIPGID,           // 6 bytes - Group ID
    DoIPByteField<DoIPFurtherAction>,  // 1 byte
    DoIPByteField<DoIPSyncStatus>      // 1 byte
>;
// Total: 33 bytes Payload

// vs. alte Methode:
static DoIPMessage makeVehicleIdentificationResponse(...) {
    ByteArray payload;
    // ... 20 Zeilen Code um Payload zu bauen
}
```

### 3. **Type-Safe Field Access** ✅

```cpp
VehicleIdentificationResponse msg(...);

// Zugriff per Index (compile-time geprüft)
const DoIPVIN& vin = msg.getField<0>();
const DoIPAddress& addr = msg.getField<1>();

// ❌ Compile-Fehler: Index out of bounds
auto invalid = msg.getField<10>();  // Nur 6 Felder!

// Ändern eines Felds (automatisches Rebuild)
msg.setField<1>(DoIPAddress(0x1234));
```

### 4. **Zero-Copy wie gehabt** ✅

```cpp
auto msg = RoutingActivationRequest(...);

// Zero-copy sending
socket.send(msg.data(), msg.size());

// Oder mit ByteArrayRef
auto data = msg.getData();
socket.send(data.first, data.second);
```

### 5. **Automatische Payload-Size Berechnung** ✅

```cpp
// Payload-Größe wird automatisch berechnet
template<DoIPField... Fields>
constexpr size_t calculatePayloadSize(const Fields&... fields) {
    return (fields.size() + ...);  // C++17 fold expression
}
```

### 6. **Parsing mit Type-Validation** ✅

```cpp
// Empfangene Nachricht parsen
auto msg = RoutingActivationRequest::parse(buffer, length);

if (msg) {
    // Typsichere Verwendung
    DoIPAddress addr = msg->getField<0>();
    auto actType = msg->getField<1>().getValue();
}
```

## Vergleich: Alt vs. Neu

### Message Definition

**Alt (Factory-Methoden):**
```cpp
static DoIPMessage makeRoutingRequest(const DoIPAddress &ea, 
                                      DoIPActivationType actType) {
    ByteArray payload;
    payload.reserve(ea.size() + 1 + 4);
    ea.appendTo(payload);
    payload.emplace_back(static_cast<uint8_t>(actType));
    payload.insert(payload.end(), {0, 0, 0, 0});  // Reserved
    
    return DoIPMessage(DoIPPayloadType::RoutingActivationRequest, payload);
}
```

**Neu (Variadic Template):**
```cpp
using RoutingActivationRequest = DoIPMessage<
    DoIPPayloadType::RoutingActivationRequest,
    DoIPAddress,
    DoIPByteField<DoIPActivationType>,
    DoIPReserved<4>
>;

// Verwendung
RoutingActivationRequest msg(
    DoIPAddress(0x0E80),
    DoIPByteField<DoIPActivationType>(DoIPActivationType::Default),
    DoIPReserved<4>()
);
```

### Field Access

**Alt:**
```cpp
DoIPMessage msg = makeRoutingRequest(...);

// Manuelle Extraktion aus Payload
auto payload = msg.getPayload();
uint16_t addr = (payload[0] << 8) | payload[1];  // Error-prone!
uint8_t actType = payload[2];
```

**Neu:**
```cpp
RoutingActivationRequest msg(...);

// Typsicherer Zugriff
DoIPAddress addr = msg.getField<0>();
DoIPActivationType actType = msg.getField<1>().getValue();
```

### Modification

**Alt:**
```cpp
// Gesamte Message neu bauen
auto newMsg = makeRoutingRequest(newAddr, newActType);
```

**Neu:**
```cpp
// Einzelnes Feld ändern (automatisches Rebuild)
msg.setField<0>(DoIPAddress(0x1234));
```

## Implementierungs-Details

### 1. Field Concept (C++20)

```cpp
template<typename T>
concept DoIPField = requires(T t, ByteArray& arr) {
    { t.size() } -> std::convertible_to<size_t>;
    { t.appendTo(arr) } -> std::same_as<void>;
};
```

Jedes Feld muss implementieren:
- `size_t size() const` - Größe in Bytes
- `void appendTo(ByteArray&) const` - Daten anhängen

### 2. Variadic Template

```cpp
template<DoIPPayloadType PayloadTypeValue, DoIPField... Fields>
class DoIPMessage {
    std::tuple<Fields...> m_fields;  // Alle Felder als Tuple
    ByteArray m_data;                // Komplette Nachricht
    
    // ...
};
```

### 3. Fold Expressions (C++17)

```cpp
// Summe aller Field-Größen
constexpr size_t calculatePayloadSize(const Fields&... fields) {
    return (fields.size() + ...);
}

// Alle Felder anhängen
void appendAllFields(ByteArray& arr, const Fields&... fields) {
    (fields.appendTo(arr), ...);
}
```

### 4. Index Sequence für Parsing

```cpp
template<size_t... Indices>
static std::optional<DoIPMessage> parseFields(
    const uint8_t* data, size_t length, std::index_sequence<Indices...>) {
    
    std::tuple<Fields...> fields;
    bool success = (parseField<Indices>(fields, data, length, offset) && ...);
    // ...
}
```

## Anforderungen an Field-Klassen

Jede Klasse, die als Field verwendet werden soll, muss implementieren:

```cpp
class MyField {
public:
    // Konstruktor aus Raw-Daten (für Parsing)
    explicit MyField(const uint8_t* data, size_t offset);
    
    // Größe in Bytes
    size_t size() const;
    
    // Daten anhängen
    void appendTo(ByteArray& arr) const;
};
```

### Beispiel: DoIPAddress

```cpp
class DoIPAddress {
public:
    explicit DoIPAddress(uint16_t addr) : m_address(addr) {}
    
    // Für Parsing
    explicit DoIPAddress(const uint8_t* data, size_t offset) 
        : m_address((data[offset] << 8) | data[offset + 1]) {}
    
    size_t size() const { return 2; }
    
    void appendTo(ByteArray& arr) const {
        arr.emplace_back((m_address >> 8) & 0xFF);
        arr.emplace_back(m_address & 0xFF);
    }
    
    uint16_t value() const { return m_address; }
    
private:
    uint16_t m_address;
};
```

## Helper-Typen

### DoIPReserved<N>
Für reservierte Bytes:
```cpp
template<size_t N>
class DoIPReserved {
    ByteArray m_data{N, 0x00};  // N Nullen
    // ...
};

// Verwendung
using MyMessage = DoIPMessage<
    PayloadType::Something,
    DoIPAddress,
    DoIPReserved<4>  // 4 reserved bytes
>;
```

### DoIPByteField<EnumType>
Für Enum-Werte:
```cpp
template<typename EnumType>
class DoIPByteField {
    uint8_t m_value;
    // ...
};

// Verwendung
DoIPByteField<DoIPActivationType> actType(DoIPActivationType::Default);
```

### DoIPVariablePayload
Für variable Daten (z.B. UDS-Payload):
```cpp
class DoIPVariablePayload {
    ByteArray m_data;
    // ...
};

// Verwendung in Diagnostic Message
using DiagnosticMessage = DoIPMessage<
    DoIPPayloadType::DiagnosticMessage,
    DoIPAddress,         // Source
    DoIPAddress,         // Target
    DoIPVariablePayload  // UDS data
>;
```

## Nachrichtentypen-Definitionen

```cpp
namespace doip {

// Einfache Nachrichten (keine Felder)
using VehicleIdentificationRequest = DoIPMessage<
    DoIPPayloadType::VehicleIdentificationRequest
>;

using AliveCheckRequest = DoIPMessage<
    DoIPPayloadType::AliveCheckRequest
>;

// Nachrichten mit Feldern
using AliveCheckResponse = DoIPMessage<
    DoIPPayloadType::AliveCheckResponse,
    DoIPAddress
>;

using RoutingActivationRequest = DoIPMessage<
    DoIPPayloadType::RoutingActivationRequest,
    DoIPAddress,
    DoIPByteField<DoIPActivationType>,
    DoIPReserved<4>
>;

using DiagnosticMessage = DoIPMessage<
    DoIPPayloadType::DiagnosticMessage,
    DoIPAddress,
    DoIPAddress,
    DoIPVariablePayload
>;

using VehicleIdentificationResponse = DoIPMessage<
    DoIPPayloadType::VehicleIdentificationResponse,
    DoIPVIN,
    DoIPAddress,
    DoIPEID,
    DoIPGID,
    DoIPByteField<DoIPFurtherAction>,
    DoIPByteField<DoIPSyncStatus>
>;

} // namespace doip
```

## Performance

### Compile-Time
- ✅ Alle Type-Checks zur Compile-Zeit
- ✅ Keine Runtime-Overhead für Type-Validation
- ✅ Optimierte Code-Generierung durch Template-Expansion

### Runtime
- ✅ Zero-Copy (wie vorher)
- ✅ Effiziente Memory-Layout (std::tuple)
- ✅ Nur ein malloc für m_data
- ⚠️  Tuple-Zugriff hat minimalen Overhead (meist weg-optimiert)

### Memory
```
DoIPMessage<PayloadType, Field1, Field2, Field3>
├── std::tuple<Field1, Field2, Field3>  // ~Summe der Field-Größen
└── ByteArray m_data                    // 8 + Payload Size

Total: sizeof(tuple) + sizeof(ByteArray) + payload_size
```

## Herausforderungen & Lösungen

### 1. Variable-Length Fields (z.B. UDS-Payload)

**Problem:** Wie kennt ein Field seine Größe beim Parsing?

**Lösung 1:** Variable Felder sind immer das letzte Feld
```cpp
using DiagnosticMessage = DoIPMessage<
    DoIPPayloadType::DiagnosticMessage,
    DoIPAddress,         // Fixed: 2 bytes
    DoIPAddress,         // Fixed: 2 bytes
    DoIPVariablePayload  // Variable: Rest der Nachricht
>;
```

**Lösung 2:** Length-Prefix
```cpp
class LengthPrefixedPayload {
    uint32_t m_length;
    ByteArray m_data;
    
    explicit LengthPrefixedPayload(const uint8_t* data, size_t offset) {
        m_length = /* read from data */;
        m_data.assign(data + offset + 4, data + offset + 4 + m_length);
    }
};
```

### 2. Optionale Felder

**Lösung:** std::optional als Field-Typ
```cpp
class OptionalField {
    std::optional<DoIPAddress> m_address;
    
    size_t size() const {
        return m_address.has_value() ? m_address->size() : 0;
    }
    
    void appendTo(ByteArray& arr) const {
        if (m_address.has_value()) {
            m_address->appendTo(arr);
        }
    }
};
```

### 3. C++17 vs. C++20

Das Design verwendet:
- **C++17:** Fold expressions, std::tuple, std::optional
- **C++20:** Concepts (optional, kann durch SFINAE ersetzt werden)

**Für C++17-only:**
```cpp
// Statt Concept
template<typename T>
using is_doip_field = std::conjunction<
    std::is_same<decltype(std::declval<T>().size()), size_t>,
    std::is_same<decltype(std::declval<T>().appendTo(std::declval<ByteArray&>())), void>
>;

// Verwendung
template<DoIPPayloadType PT, typename... Fields>
class DoIPMessage {
    static_assert((is_doip_field<Fields>::value && ...), "Invalid field type");
    // ...
};
```

## Migration von bestehendem Code

### Schritt 1: Field-Klassen anpassen

Bestehende Klassen müssen implementieren:
```cpp
class DoIPAddress {
    // ✅ Bereits vorhanden (wahrscheinlich)
    uint16_t value() const;
    
    // ➕ Hinzufügen
    size_t size() const { return 2; }
    void appendTo(ByteArray& arr) const {
        arr.emplace_back((m_address >> 8) & 0xFF);
        arr.emplace_back(m_address & 0xFF);
    }
    
    // ➕ Für Parsing
    explicit DoIPAddress(const uint8_t* data, size_t offset);
};
```

### Schritt 2: Message-Typen definieren

```cpp
// Alt
static DoIPMessage makeAliveCheckResponse(const DoIPAddress& sa);

// Neu
using AliveCheckResponse = DoIPMessage<
    DoIPPayloadType::AliveCheckResponse,
    DoIPAddress
>;
```

### Schritt 3: Code umstellen

```cpp
// Alt
auto msg = DoIPMessage::makeAliveCheckResponse(addr);

// Neu
AliveCheckResponse msg(addr);
```

## Fazit

### ✅ Pros
1. **Compile-time type safety** - Fehler beim Kompilieren statt zur Laufzeit
2. **Selbstdokumentierend** - Message-Struktur ist sofort ersichtlich
3. **Type-safe field access** - `getField<N>()` statt manuelle Byte-Extraktion
4. **Weniger Boilerplate** - Keine langen Factory-Funktionen mehr
5. **Zero-copy** - Wie bisherige Implementierung
6. **Erweiterbar** - Neue Message-Typen sind trivial hinzuzufügen
7. **Modern C++** - Nutzt C++17/20 Features optimal aus

### ⚠️ Cons
1. **Komplexere Template-Magie** - Schwerer zu debuggen
2. **Längere Compile-Zeiten** - Templates müssen instanziiert werden
3. **Steilere Lernkurve** - Variadic templates sind nicht trivial
4. **Field-Klassen müssen angepasst werden** - Interface implementieren
5. **Variable-length fields** - Erfordern besondere Behandlung

### 🎯 Empfehlung

**Ja, mach es!** Aber schrittweise:

1. **Phase 1:** Neue Message-Typen im neuen Stil hinzufügen
2. **Phase 2:** Field-Klassen nach und nach anpassen
3. **Phase 3:** Alte Factory-Methoden durch Aliases ersetzen
4. **Phase 4:** Alte Implementation als deprecated markieren

**Hybrid-Ansatz:**
```cpp
// Beide APIs parallel unterstützen
namespace doip {
    namespace message {
        // Alte API (deprecated)
        [[deprecated("Use RoutingActivationRequest type instead")]]
        inline DoIPMessage makeRoutingRequest(...) { /* ... */ }
    }
    
    // Neue API
    using RoutingActivationRequest = DoIPMessage<...>;
}
```

Das gibt dir das Beste aus beiden Welten! 🚀

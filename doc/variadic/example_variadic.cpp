#include "DoIPMessage_variadic.h"
#include <iostream>
#include <iomanip>

// Mock classes for demonstration (replace with your actual implementations)
namespace doip {

class DoIPAddress {
public:
    explicit DoIPAddress(uint16_t addr) : m_address(addr) {}
    explicit DoIPAddress(const uint8_t* data, size_t offset) 
        : m_address((static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1]) {}
    
    size_t size() const { return 2; }
    
    void appendTo(ByteArray& arr) const {
        arr.emplace_back((m_address >> 8) & 0xFF);
        arr.emplace_back(m_address & 0xFF);
    }
    
    uint16_t value() const { return m_address; }
    
private:
    uint16_t m_address;
};

class DoIPVIN {
public:
    explicit DoIPVIN(const std::string& vin) {
        m_vin.resize(17, 0);
        std::copy_n(vin.begin(), std::min(vin.size(), size_t(17)), m_vin.begin());
    }
    
    explicit DoIPVIN(const uint8_t* data, size_t offset) 
        : m_vin(data + offset, data + offset + 17) {}
    
    size_t size() const { return 17; }
    
    void appendTo(ByteArray& arr) const {
        arr.insert(arr.end(), m_vin.begin(), m_vin.end());
    }
    
    std::string toString() const {
        return std::string(m_vin.begin(), m_vin.end());
    }
    
private:
    ByteArray m_vin;
};

class DoIPEID {
public:
    explicit DoIPEID(const ByteArray& eid) : m_eid(eid) {
        m_eid.resize(6);
    }
    
    explicit DoIPEID(const uint8_t* data, size_t offset) 
        : m_eid(data + offset, data + offset + 6) {}
    
    size_t size() const { return 6; }
    
    void appendTo(ByteArray& arr) const {
        arr.insert(arr.end(), m_eid.begin(), m_eid.end());
    }
    
private:
    ByteArray m_eid;
};

class DoIPGID {
public:
    explicit DoIPGID(const ByteArray& gid) : m_gid(gid) {
        m_gid.resize(6);
    }
    
    explicit DoIPGID(const uint8_t* data, size_t offset) 
        : m_gid(data + offset, data + offset + 6) {}
    
    size_t size() const { return 6; }
    
    void appendTo(ByteArray& arr) const {
        arr.insert(arr.end(), m_gid.begin(), m_gid.end());
    }
    
private:
    ByteArray m_gid;
};

enum class DoIPActivationType : uint8_t {
    Default = 0x00,
    WWH_OBD = 0x01,
    CentralSecurity = 0xE0
};

enum class DoIPFurtherAction : uint8_t {
    NoFurtherAction = 0x00,
    RoutingRequired = 0x10
};

enum class DoIPSyncStatus : uint8_t {
    GidVinSynchronized = 0x00,
    NotSynchronized = 0x10
};

} // namespace doip

using namespace doip;

// Helper function to print bytes
void printBytes(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<unsigned>(data[i]) << " ";
        if ((i + 1) % 16 == 0) std::cout << "\n             ";
    }
    std::cout << std::dec << std::endl;
}

void example1_basic_usage() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 1: Basic Usage - Type-Safe Construction  ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    // 1. Vehicle Identification Request (no fields)
    std::cout << "1. Vehicle Identification Request:\n";
    VehicleIdentificationRequest vidReq;
    std::cout << "   Size: " << vidReq.size() << " bytes\n";
    std::cout << "   Data: ";
    printBytes(vidReq.data(), vidReq.size());
    
    // 2. Routing Activation Request (3 fields)
    std::cout << "\n2. Routing Activation Request:\n";
    DoIPAddress sourceAddr(0x0E80);
    DoIPByteField<DoIPActivationType> actType(DoIPActivationType::Default);
    DoIPReserved<4> reserved;
    
    RoutingActivationRequest routingReq(sourceAddr, actType, reserved);
    std::cout << "   Size: " << routingReq.size() << " bytes\n";
    std::cout << "   Data: ";
    printBytes(routingReq.data(), routingReq.size());
    
    // 3. Alive Check Request (no fields)
    std::cout << "\n3. Alive Check Request:\n";
    AliveCheckRequest aliveReq;
    std::cout << "   Size: " << aliveReq.size() << " bytes\n";
    std::cout << "   Data: ";
    printBytes(aliveReq.data(), aliveReq.size());
    
    // 4. Alive Check Response (1 field)
    std::cout << "\n4. Alive Check Response:\n";
    AliveCheckResponse aliveResp(DoIPAddress(0x0E80));
    std::cout << "   Size: " << aliveResp.size() << " bytes\n";
    std::cout << "   Data: ";
    printBytes(aliveResp.data(), aliveResp.size());
}

void example2_diagnostic_messages() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 2: Diagnostic Messages                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    DoIPAddress sourceAddr(0x0E80);
    DoIPAddress targetAddr(0x1234);
    DoIPVariablePayload udsData(ByteArray{0x10, 0x01}); // DiagnosticSessionControl
    
    // Create diagnostic message
    DiagnosticMessage diagMsg(sourceAddr, targetAddr, std::move(udsData));
    
    std::cout << "Diagnostic Message:\n";
    std::cout << "   Payload Type: " << static_cast<int>(diagMsg.getPayloadType()) << "\n";
    std::cout << "   Total Size: " << diagMsg.size() << " bytes\n";
    std::cout << "   Payload Size: " << diagMsg.getPayloadSize() << " bytes\n";
    std::cout << "   Data: ";
    printBytes(diagMsg.data(), diagMsg.size());
    
    // Access fields directly
    std::cout << "\n   Field Access:\n";
    std::cout << "   - Source Address: 0x" << std::hex 
              << diagMsg.getField<0>().value() << std::dec << "\n";
    std::cout << "   - Target Address: 0x" << std::hex 
              << diagMsg.getField<1>().value() << std::dec << "\n";
    std::cout << "   - UDS Payload Size: " 
              << diagMsg.getField<2>().size() << " bytes\n";
}

void example3_complex_message() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 3: Complex Message - Vehicle ID Response ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    // Create all fields
    DoIPVIN vin("WAUZZZ8V9KA123456");
    DoIPAddress logicalAddr(0x0E80);
    DoIPEID eid(ByteArray{0x00, 0x11, 0x22, 0x33, 0x44, 0x55});
    DoIPGID gid(ByteArray{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF});
    DoIPByteField<DoIPFurtherAction> furtherAction(DoIPFurtherAction::NoFurtherAction);
    DoIPByteField<DoIPSyncStatus> syncStatus(DoIPSyncStatus::GidVinSynchronized);
    
    // Create message with all fields
    VehicleIdentificationResponse vidResp(
        vin, logicalAddr, eid, gid, furtherAction, syncStatus
    );
    
    std::cout << "Vehicle Identification Response:\n";
    std::cout << "   Total Size: " << vidResp.size() << " bytes\n";
    std::cout << "   Payload Size: " << vidResp.getPayloadSize() << " bytes\n";
    std::cout << "   Number of Fields: " << vidResp.FIELD_COUNT << "\n";
    std::cout << "   Data: ";
    printBytes(vidResp.data(), vidResp.size());
    
    // Access individual fields
    std::cout << "\n   Field Access:\n";
    std::cout << "   - VIN: " << vidResp.getField<0>().toString() << "\n";
    std::cout << "   - Logical Address: 0x" << std::hex 
              << vidResp.getField<1>().value() << std::dec << "\n";
    std::cout << "   - Further Action: 0x" << std::hex 
              << static_cast<int>(vidResp.getField<4>().getRawValue()) << std::dec << "\n";
    std::cout << "   - Sync Status: 0x" << std::hex 
              << static_cast<int>(vidResp.getField<5>().getRawValue()) << std::dec << "\n";
}

void example4_field_modification() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 4: Field Modification                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    // Create initial message
    AliveCheckResponse aliveResp(DoIPAddress(0x0E80));
    
    std::cout << "Original message:\n";
    std::cout << "   Address: 0x" << std::hex 
              << aliveResp.getField<0>().value() << std::dec << "\n";
    std::cout << "   Data: ";
    printBytes(aliveResp.data(), aliveResp.size());
    
    // Modify field
    std::cout << "\nModifying address to 0x1234...\n";
    aliveResp.setField<0>(DoIPAddress(0x1234));
    
    std::cout << "\nModified message:\n";
    std::cout << "   Address: 0x" << std::hex 
              << aliveResp.getField<0>().value() << std::dec << "\n";
    std::cout << "   Data: ";
    printBytes(aliveResp.data(), aliveResp.size());
}

void example5_parsing() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 5: Parsing Received Messages            ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    // Simulate received data
    uint8_t receivedData[] = {
        0x04,       // Protocol version
        0xFB,       // Inverse
        0x00, 0x08, // Payload type (AliveCheckResponse = 0x0008)
        0x00, 0x00, 0x00, 0x02, // Payload length (2 bytes)
        0x0E, 0x80  // Source address
    };
    
    std::cout << "Received data:\n   ";
    printBytes(receivedData, sizeof(receivedData));
    
    // Parse message
    auto msg = AliveCheckResponse::parse(receivedData, sizeof(receivedData));
    
    if (msg) {
        std::cout << "\n✅ Parsing successful!\n";
        std::cout << "   Payload Type: 0x" << std::hex 
                  << static_cast<int>(msg->getPayloadType()) << std::dec << "\n";
        std::cout << "   Source Address: 0x" << std::hex 
                  << msg->getField<0>().value() << std::dec << "\n";
    } else {
        std::cout << "\n❌ Parsing failed!\n";
    }
}

void example6_compile_time_safety() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 6: Compile-Time Type Safety             ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Compile-time information:\n";
    
    std::cout << "\nVehicleIdentificationRequest:\n";
    std::cout << "   - Payload Type: 0x" << std::hex 
              << static_cast<int>(VehicleIdentificationRequest::PAYLOAD_TYPE) << std::dec << "\n";
    std::cout << "   - Field Count: " << VehicleIdentificationRequest::FIELD_COUNT << "\n";
    std::cout << "   - Header Size: " << VehicleIdentificationRequest::HEADER_SIZE << " bytes\n";
    
    std::cout << "\nRoutingActivationRequest:\n";
    std::cout << "   - Payload Type: 0x" << std::hex 
              << static_cast<int>(RoutingActivationRequest::PAYLOAD_TYPE) << std::dec << "\n";
    std::cout << "   - Field Count: " << RoutingActivationRequest::FIELD_COUNT << "\n";
    
    std::cout << "\nDiagnosticMessage:\n";
    std::cout << "   - Payload Type: 0x" << std::hex 
              << static_cast<int>(DiagnosticMessage::PAYLOAD_TYPE) << std::dec << "\n";
    std::cout << "   - Field Count: " << DiagnosticMessage::FIELD_COUNT << "\n";
    
    std::cout << "\nVehicleIdentificationResponse:\n";
    std::cout << "   - Payload Type: 0x" << std::hex 
              << static_cast<int>(VehicleIdentificationResponse::PAYLOAD_TYPE) << std::dec << "\n";
    std::cout << "   - Field Count: " << VehicleIdentificationResponse::FIELD_COUNT << "\n";
    
    // The following would cause compile errors:
    // auto invalid = AliveCheckResponse(DoIPVIN("test")); // ❌ Wrong field type
    // auto outOfBounds = aliveResp.getField<5>(); // ❌ Field index out of bounds
    
    std::cout << "\n✅ All type checks happen at compile time!\n";
    std::cout << "   - Wrong field types are rejected by the compiler\n";
    std::cout << "   - Out-of-bounds field access is caught at compile time\n";
    std::cout << "   - Payload types are validated at compile time\n";
}

void example7_zero_copy_sending() {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║  Example 7: Zero-Copy Sending                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    // Create message
    AliveCheckResponse msg(DoIPAddress(0x0E80));
    
    std::cout << "Sending message (3 different methods):\n\n";
    
    // Method 1: ByteArrayRef
    std::cout << "1. Using ByteArrayRef:\n";
    auto dataRef = msg.getData();
    std::cout << "   Pointer: " << static_cast<const void*>(dataRef.first) << "\n";
    std::cout << "   Size: " << dataRef.second << " bytes\n";
    std::cout << "   // socket.send(dataRef.first, dataRef.second);\n";
    
    // Method 2: Direct pointer + size
    std::cout << "\n2. Using data() + size():\n";
    std::cout << "   Pointer: " << static_cast<const void*>(msg.data()) << "\n";
    std::cout << "   Size: " << msg.size() << " bytes\n";
    std::cout << "   // socket.send(msg.data(), msg.size());\n";
    
    // Method 3: Template function
    std::cout << "\n3. Using template function:\n";
    auto sendMessage = [](const auto& message) {
        std::cout << "   Sending " << message.size() << " bytes...\n";
        std::cout << "   // send(message.data(), message.size());\n";
    };
    sendMessage(msg);
    
    std::cout << "\n✅ All methods provide zero-copy access to the message data!\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║       DoIPMessage - Variadic Template Demo        ║\n";
    std::cout << "║              Type-Safe Message Builder            ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    example1_basic_usage();
    example2_diagnostic_messages();
    example3_complex_message();
    example4_field_modification();
    example5_parsing();
    example6_compile_time_safety();
    example7_zero_copy_sending();
    
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                 Key Advantages                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    std::cout << "✅ Compile-time type safety (wrong types = compile error)\n";
    std::cout << "✅ Zero-copy message access\n";
    std::cout << "✅ Clean, declarative message definitions\n";
    std::cout << "✅ Automatic payload size calculation\n";
    std::cout << "✅ Type-safe field access by index\n";
    std::cout << "✅ Automatic message (re)building on field changes\n";
    std::cout << "✅ Parsing support with type validation\n";
    std::cout << "✅ No runtime overhead (everything resolved at compile time)\n";
    
    return 0;
}

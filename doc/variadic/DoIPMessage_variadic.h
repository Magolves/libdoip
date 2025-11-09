#ifndef DOIPMESSAGE_VARIADIC_H
#define DOIPMESSAGE_VARIADIC_H

#include <stdint.h>
#include <vector>
#include <tuple>
#include <type_traits>
#include <optional>

#include "ByteArray.h"
#include "DoIPPayloadType.h"

namespace doip {

/**
 * @brief Concept to check if a type has writeToArray method
 * 
 * A DoIP field type must provide:
 * - size_t size() const
 * - void appendTo(ByteArray&) const
 */
template<typename T>
concept DoIPField = requires(T t, ByteArray& arr) {
    { t.size() } -> std::convertible_to<size_t>;
    { t.appendTo(arr) } -> std::same_as<void>;
};

/**
 * @brief Concept for types that can be read from ByteArray
 */
template<typename T>
concept DoIPReadableField = DoIPField<T> && requires(const uint8_t* data, size_t offset) {
    { T(data, offset) } -> std::convertible_to<T>;
};

/**
 * @brief Helper to calculate total size of all fields at compile time
 */
template<DoIPField... Fields>
constexpr size_t calculatePayloadSize(const Fields&... fields) {
    return (fields.size() + ...);
}

/**
 * @brief Helper to append all fields to ByteArray
 */
template<DoIPField... Fields>
void appendAllFields(ByteArray& arr, const Fields&... fields) {
    (fields.appendTo(arr), ...);
}

/**
 * @brief Base DoIP Message with compile-time type safety
 * 
 * Template parameters:
 * @tparam PayloadTypeValue The DoIPPayloadType for this message
 * @tparam Fields Types of the fields in the payload (must satisfy DoIPField concept)
 * 
 * Example:
 * @code
 * // Routing Activation Request: source address (2 bytes) + activation type (1 byte) + reserved (4 bytes)
 * using RoutingActivationRequest = DoIPMessage<
 *     DoIPPayloadType::RoutingActivationRequest,
 *     DoIPAddress,        // Source address
 *     DoIPActivationType, // Activation type
 *     DoIPReserved<4>     // 4 reserved bytes
 * >;
 * @endcode
 */
template<DoIPPayloadType PayloadTypeValue, DoIPField... Fields>
class DoIPMessage {
public:
    static constexpr DoIPPayloadType PAYLOAD_TYPE = PayloadTypeValue;
    static constexpr size_t HEADER_SIZE = 8;
    static constexpr size_t FIELD_COUNT = sizeof...(Fields);
    
    /**
     * @brief Construct message from field values
     */
    explicit DoIPMessage(Fields... fields) 
        : m_fields(std::move(fields)...) {
        buildMessage();
    }
    
    /**
     * @brief Get the complete message data (zero-copy)
     */
    ByteArrayRef getData() const {
        return {m_data.data(), m_data.size()};
    }
    
    /**
     * @brief Get raw pointer to message data
     */
    const uint8_t* data() const {
        return m_data.data();
    }
    
    /**
     * @brief Get total message size
     */
    size_t size() const {
        return m_data.size();
    }
    
    /**
     * @brief Get payload type
     */
    constexpr DoIPPayloadType getPayloadType() const {
        return PAYLOAD_TYPE;
    }
    
    /**
     * @brief Get payload size (without header)
     */
    size_t getPayloadSize() const {
        return m_data.size() - HEADER_SIZE;
    }
    
    /**
     * @brief Access individual field by index
     * 
     * @tparam Index Field index (0-based)
     * @return const reference to the field
     */
    template<size_t Index>
    const auto& getField() const {
        static_assert(Index < FIELD_COUNT, "Field index out of bounds");
        return std::get<Index>(m_fields);
    }
    
    /**
     * @brief Access individual field by index
     * 
     * @tparam Index Field index (0-based)
     * @return reference to the field (allows modification)
     */
    template<size_t Index>
    auto& getField() {
        static_assert(Index < FIELD_COUNT, "Field index out of bounds");
        return std::get<Index>(m_fields);
    }
    
    /**
     * @brief Update a field and rebuild the message
     * 
     * @tparam Index Field index
     * @param newValue New value for the field
     */
    template<size_t Index>
    void setField(const std::tuple_element_t<Index, std::tuple<Fields...>>& newValue) {
        std::get<Index>(m_fields) = newValue;
        buildMessage();
    }
    
    /**
     * @brief Parse a message from raw data
     * 
     * @param data Raw message data
     * @param length Length of the data
     * @return Optional message if parsing succeeds
     */
    static std::optional<DoIPMessage> parse(const uint8_t* data, size_t length) 
        requires (DoIPReadableField<Fields> && ...) 
    {
        if (!data || length < HEADER_SIZE) {
            return std::nullopt;
        }
        
        // Validate header
        if (!validateHeader(data, length)) {
            return std::nullopt;
        }
        
        // Parse fields
        return parseFields(data + HEADER_SIZE, length - HEADER_SIZE, std::index_sequence_for<Fields...>{});
    }
    
protected:
    std::tuple<Fields...> m_fields; ///< The message fields
    ByteArray m_data;               ///< Complete message (header + payload)
    
    /**
     * @brief Build the complete message from fields
     */
    void buildMessage() {
        // Calculate total size
        size_t payloadSize = std::apply(
            [](const auto&... fields) { return calculatePayloadSize(fields...); },
            m_fields
        );
        
        // Reserve space
        m_data.clear();
        m_data.reserve(HEADER_SIZE + payloadSize);
        
        // Build header
        buildHeader(payloadSize);
        
        // Append fields
        std::apply(
            [this](const auto&... fields) { appendAllFields(m_data, fields...); },
            m_fields
        );
    }
    
    /**
     * @brief Build the DoIP header
     */
    void buildHeader(size_t payloadSize) {
        constexpr uint8_t PROTOCOL_VERSION = 0x04;
        constexpr uint8_t PROTOCOL_VERSION_INV = 0xFB;
        
        // Protocol version
        m_data.emplace_back(PROTOCOL_VERSION);
        m_data.emplace_back(PROTOCOL_VERSION_INV);
        
        // Payload type
        uint16_t typeValue = static_cast<uint16_t>(PAYLOAD_TYPE);
        m_data.emplace_back((typeValue >> 8) & 0xFF);
        m_data.emplace_back(typeValue & 0xFF);
        
        // Payload length
        uint32_t size = static_cast<uint32_t>(payloadSize);
        m_data.emplace_back((size >> 24) & 0xFF);
        m_data.emplace_back((size >> 16) & 0xFF);
        m_data.emplace_back((size >> 8) & 0xFF);
        m_data.emplace_back(size & 0xFF);
    }
    
    /**
     * @brief Validate the DoIP header
     */
    static bool validateHeader(const uint8_t* data, size_t length) {
        if (length < HEADER_SIZE) return false;
        
        // Check protocol version
        if (data[0] < 0x01 || data[0] > 0x04 || data[0] + data[1] != 0xFF) {
            return false;
        }
        
        // Check payload type
        uint16_t typeValue = (static_cast<uint16_t>(data[2]) << 8) | data[3];
        if (static_cast<DoIPPayloadType>(typeValue) != PAYLOAD_TYPE) {
            return false;
        }
        
        // Check payload length
        uint32_t b3 = static_cast<uint32_t>(data[4]) << 24;
        uint32_t b2 = static_cast<uint32_t>(data[5]) << 16;
        uint32_t b1 = static_cast<uint32_t>(data[6]) << 8;
        uint32_t b0 = static_cast<uint32_t>(data[7]);
        size_t payloadLength = static_cast<size_t>(b3 | b2 | b1 | b0);
        
        return length >= HEADER_SIZE + payloadLength;
    }
    
    /**
     * @brief Parse fields from payload data
     */
    template<size_t... Indices>
    static std::optional<DoIPMessage> parseFields(const uint8_t* data, size_t length, 
                                                   std::index_sequence<Indices...>) {
        size_t offset = 0;
        
        // Parse each field
        std::tuple<Fields...> fields;
        bool success = (parseField<Indices>(fields, data, length, offset) && ...);
        
        if (!success) {
            return std::nullopt;
        }
        
        // Create message from parsed fields
        return std::apply(
            [](auto&&... args) { return DoIPMessage(std::forward<decltype(args)>(args)...); },
            std::move(fields)
        );
    }
    
    /**
     * @brief Parse a single field
     */
    template<size_t Index>
    static bool parseField(std::tuple<Fields...>& fields, const uint8_t* data, 
                          size_t length, size_t& offset) {
        using FieldType = std::tuple_element_t<Index, std::tuple<Fields...>>;
        
        if (offset >= length) {
            return false;
        }
        
        // Construct field from data
        FieldType field(data, offset);
        
        // Update offset
        offset += field.size();
        
        if (offset > length) {
            return false;
        }
        
        // Store field
        std::get<Index>(fields) = std::move(field);
        
        return true;
    }
};

// ============================================================================
// Helper types for common DoIP fields
// ============================================================================

/**
 * @brief Reserved bytes field (compile-time size)
 */
template<size_t N>
class DoIPReserved {
public:
    DoIPReserved() : m_data(N, 0x00) {}
    explicit DoIPReserved(const uint8_t* data, size_t /*offset*/) : m_data(data, data + N) {}
    
    size_t size() const { return N; }
    
    void appendTo(ByteArray& arr) const {
        arr.insert(arr.end(), m_data.begin(), m_data.end());
    }
    
private:
    ByteArray m_data;
};

/**
 * @brief Single byte field wrapper
 */
template<typename EnumType>
class DoIPByteField {
public:
    explicit DoIPByteField(EnumType value) : m_value(static_cast<uint8_t>(value)) {}
    explicit DoIPByteField(uint8_t value) : m_value(value) {}
    explicit DoIPByteField(const uint8_t* data, size_t offset) : m_value(data[offset]) {}
    
    size_t size() const { return 1; }
    
    void appendTo(ByteArray& arr) const {
        arr.emplace_back(m_value);
    }
    
    EnumType getValue() const { return static_cast<EnumType>(m_value); }
    uint8_t getRawValue() const { return m_value; }
    
private:
    uint8_t m_value;
};

/**
 * @brief Variable-length payload field (e.g., UDS data)
 */
class DoIPVariablePayload {
public:
    DoIPVariablePayload() = default;
    explicit DoIPVariablePayload(const ByteArray& data) : m_data(data) {}
    explicit DoIPVariablePayload(ByteArray&& data) : m_data(std::move(data)) {}
    
    // Note: This requires knowing the remaining length when parsing
    explicit DoIPVariablePayload(const uint8_t* data, size_t offset, size_t remaining) 
        : m_data(data + offset, data + offset + remaining) {}
    
    size_t size() const { return m_data.size(); }
    
    void appendTo(ByteArray& arr) const {
        arr.insert(arr.end(), m_data.begin(), m_data.end());
    }
    
    const ByteArray& getData() const { return m_data; }
    ByteArray& getData() { return m_data; }
    
private:
    ByteArray m_data;
};

// ============================================================================
// Example: Concrete message types
// ============================================================================

// Forward declarations (assuming these exist in your codebase)
class DoIPAddress;
class DoIPVIN;
class DoIPEID;
class DoIPGID;
enum class DoIPActivationType : uint8_t;
enum class DoIPFurtherAction : uint8_t;
enum class DoIPSyncStatus : uint8_t;

/**
 * @brief Vehicle Identification Request (no payload)
 * Table 4, ISO 13400-2
 */
using VehicleIdentificationRequest = DoIPMessage<
    DoIPPayloadType::VehicleIdentificationRequest
>;

/**
 * @brief Vehicle Identification Response
 * Table 5, ISO 13400-2
 * 
 * Payload structure:
 * - VIN (17 bytes)
 * - Logical Address (2 bytes)
 * - EID (6 bytes)
 * - GID (6 bytes)
 * - Further Action (1 byte)
 * - Sync Status (1 byte)
 */
using VehicleIdentificationResponse = DoIPMessage<
    DoIPPayloadType::VehicleIdentificationResponse,
    DoIPVIN,
    DoIPAddress,
    DoIPEID,
    DoIPGID,
    DoIPByteField<DoIPFurtherAction>,
    DoIPByteField<DoIPSyncStatus>
>;

/**
 * @brief Routing Activation Request
 * Table 13, ISO 13400-2
 * 
 * Payload structure:
 * - Source Address (2 bytes)
 * - Activation Type (1 byte)
 * - Reserved (4 bytes)
 */
using RoutingActivationRequest = DoIPMessage<
    DoIPPayloadType::RoutingActivationRequest,
    DoIPAddress,
    DoIPByteField<DoIPActivationType>,
    DoIPReserved<4>
>;

/**
 * @brief Diagnostic Message
 * Table 21, ISO 13400-2
 * 
 * Payload structure:
 * - Source Address (2 bytes)
 * - Target Address (2 bytes)
 * - User Data (variable length)
 */
using DiagnosticMessage = DoIPMessage<
    DoIPPayloadType::DiagnosticMessage,
    DoIPAddress,
    DoIPAddress,
    DoIPVariablePayload
>;

/**
 * @brief Alive Check Request (no payload)
 * Table 27, ISO 13400-2
 */
using AliveCheckRequest = DoIPMessage<
    DoIPPayloadType::AliveCheckRequest
>;

/**
 * @brief Alive Check Response
 * Table 28, ISO 13400-2
 * 
 * Payload structure:
 * - Source Address (2 bytes)
 */
using AliveCheckResponse = DoIPMessage<
    DoIPPayloadType::AliveCheckResponse,
    DoIPAddress
>;

} // namespace doip

#endif /* DOIPMESSAGE_VARIADIC_H */

#ifndef DOIPMESSAGE_H
#define DOIPMESSAGE_H

#include <stdint.h>
#include <vector>

#include "DoIPPayloadType.h"

/**
 * @brief ISO/DIS 13400-2:2010
 */
constexpr uint8_t ISO_13400_2010 = 1;

/**
 * @brief ISO 13400-2:2012
 */
constexpr uint8_t ISO_13400_2012 = 2;

/**
 * @brief ISO 13400-2:2019
 */
constexpr uint8_t ISO_13400_2019 = 3;

/**
 * @brief ISO 13400-2:2019/Amd1, ISO 13400-2:2025
 */
constexpr uint8_t ISO_13400_2025 = 4;


constexpr uint8_t PROTOCOL_VERSION = ISO_13400_2025;
constexpr uint8_t PROTOCOL_VERSION_INV = static_cast<uint8_t>(~ISO_13400_2025);

using ByteArray = std::vector<uint8_t>;

using DoIPPayload = ByteArray;

/**
 * @brief Represents a DoIP message with payload type and data.
 *
 * This structure encapsulates a complete DoIP message consisting of a payload type
 * and the associated payload data as a sequence of bytes.
 *
 * createDiagnosticRequest(uint16_t source_addr, uint16_t target_addr,
                                   const std::vector<uint8_t>& service_data) {
    // Payload: [source_addr_high, source_addr_low, target_addr_high, target_addr_low, ...service_data]
    DoIPPayload payload;
    payload.emplace_back((source_addr >> 8) & 0xFF);  // HSB
    payload.emplace_back(source_addr & 0xFF);         // LSB
    payload.emplace_back((target_addr >> 8) & 0xFF);  // HSB
    payload.emplace_back(target_addr & 0xFF);         // LSB
    payload.insert(payload.end(), service_data.begin(), service_data.end());

    return DoIPMessage(DoIPPayloadType::DIAGNOSTIC_MESSAGE, std::move(payload));
}
 */
struct DoIPMessage {
    /**
     * @brief Size of the DoIP header
     */
    static constexpr size_t DOIP_HEADER_SIZE = 8;

    /**
     * @brief Constructs a DoIP message with payload type and data.
     *
     * @param pl_type The payload type for this message
     * @param payload The payload data as a vector of bytes
     */
    DoIPMessage(DoIPPayloadType pl_type, const DoIPPayload& payload)
        : m_payload_type(pl_type), m_payload(payload) {}

    /**
     * @brief Constructs a DoIP message with payload type and data (move semantics).
     *
     * @param pl_type The payload type for this message
     * @param payload The payload data as a vector of bytes (moved)
     */
    DoIPMessage(DoIPPayloadType pl_type, DoIPPayload&& payload)
        : m_payload_type(pl_type), m_payload(std::move(payload)) {}

    /**
     * @brief Constructs a DoIP message with payload type and initializer list.
     *
     * @param pl_type The payload type for this message
     * @param init_list Initializer list of bytes for the payload
     *
     * @example
     * DoIPMessage msg(DoIPPayloadType::DIAGNOSTIC_MESSAGE, {0x01, 0x02, 0x03, 0xFF});
     */
    DoIPMessage(DoIPPayloadType pl_type, std::initializer_list<uint8_t> init_list)
        : m_payload_type(pl_type), m_payload(init_list) {}

    /**
     * @brief Constructs a DoIP message from raw byte array.
     *
     * @param pl_type The payload type for this message
     * @param data Pointer to raw byte data
     * @param size Number of bytes to copy from data
     *
     * @warning No bounds checking is performed. Ensure data points to at least size bytes.
     */
    DoIPMessage(DoIPPayloadType pl_type, const uint8_t* data, size_t size)
        : m_payload_type(pl_type), m_payload(data, data + size) {}

    /**
     * @brief Constructs a DoIP message from iterator range.
     *
     * @tparam Iterator Input iterator type
     * @param pl_type The payload type for this message
     * @param first Iterator to the beginning of the data range
     * @param last Iterator to the end of the data range
     */
    template<typename Iterator>
    DoIPMessage(DoIPPayloadType pl_type, Iterator first, Iterator last)
        : m_payload_type(pl_type), m_payload(first, last) {}

    /**
     * @brief Gets the payload type of this message.
     *
     * @return The DoIP payload type
     */
    DoIPPayloadType getPayloadType() const { return m_payload_type; }

    /**
     * @brief Gets the payload data.
     *
     * @return Const reference to the payload vector
     */
    const DoIPPayload& getPayload() const { return m_payload; }

    /**
     * @brief Gets the payload size in bytes.
     *
     * @return size_t Number of bytes in the payload
     */
    size_t getPayloadSize() const { return m_payload.size(); }

    /**
     * @brief Get the Message Size object
     *
     * @return size_t Number of bytes of the message (header + payload)
     */
    size_t getMessageSize() const { return getPayloadSize() + DOIP_HEADER_SIZE; }

    /**
     * @brief Gets the complete DoIP messages as byte array.
     *
     * @return ByteArray the DoIP message bytes
     */
    ByteArray toBytes() {
        ByteArray bytes;
        bytes.reserve(getMessageSize());  // Pre-allocate memory but keep size = 0

        bytes.emplace_back(PROTOCOL_VERSION);
        bytes.emplace_back(PROTOCOL_VERSION_INV);

        appendPayloadType(bytes);
        appendSize(bytes);
        bytes.insert(bytes.end(), m_payload.begin(), m_payload.end());

        return bytes;
    }

private:
    DoIPPayloadType m_payload_type; ///< The type of DoIP payload
    DoIPPayload m_payload;          ///< The payload data as byte vector


    inline void appendSize(ByteArray& bytes) {
        uint32_t size = static_cast<uint32_t>(m_payload.size());

        bytes.emplace_back(size >> 24 & 0xff);
        bytes.emplace_back(size >> 16 & 0xff);
        bytes.emplace_back(size >> 8 & 0xff);
        bytes.emplace_back(size & 0xff);
    }

    inline void appendPayloadType(ByteArray& bytes) {
        uint16_t plt = static_cast<uint16_t>(m_payload_type);
        bytes.emplace_back(plt >> 8 & 0xff);
        bytes.emplace_back(plt & 0xff);
    }
};


#endif  /* DOIPMESSAGE_H */

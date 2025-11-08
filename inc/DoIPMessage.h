#ifndef DOIPMESSAGE_H
#define DOIPMESSAGE_H

#include <optional>
#include <stdint.h>
#include <iostream>
#include <iomanip>

#include "ByteArray.h"
#include "DoIPAddress.h"
#include "DoIPActivationType.h"
#include "DoIPNegativeAck.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPPayloadType.h"
#include "DoIPIdentifiers.h"
#include "DoIPFurtherAction.h"
#include "DoIPSyncStatus.h"

namespace doip {

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

/**
 * @brief Current protocol version (table 16)
 */
constexpr uint8_t PROTOCOL_VERSION = ISO_13400_2025;
constexpr uint8_t PROTOCOL_VERSION_INV = static_cast<uint8_t>(~ISO_13400_2025);

/**
 * @brief Positive ack for diagnostic message (table 24)
 *
 */
constexpr uint8_t DIAGNOSTIC_MESSAGE_ACK = 0;

using DoIPPayload = ByteArray;

struct DoIPMessageHeader {
        /**
     * @brief Size of the DoIP header
     */
    static constexpr size_t DOIP_HEADER_SIZE = 8;


    static std::optional<std::pair<DoIPPayloadType, uint32_t>> parseHeader(const uint8_t *data, size_t length) {
        if (!data || length < DoIPMessageHeader::DOIP_HEADER_SIZE) return std::nullopt;

        if (!isValidProtocolVersion(data, length)) {
            return std::nullopt;
        }

        auto plt = getPayloadType(data, length);
        if (!plt.has_value()) {
            return std::nullopt;
        }

        size_t payloadLength = static_cast<size_t>(data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

        return std::make_pair(plt.value(), payloadLength);
    }

    static bool isValidProtocolVersion(const uint8_t *data, size_t length) {
        if (data == nullptr || length < DoIPMessageHeader::DOIP_HEADER_SIZE) {
            return false; // message null or too short
        }

        return data[0] >= ISO_13400_2010 && // check minimum protocol version
               data[0] <= ISO_13400_2025 && // check maximum (known) protocol version
               data[0] + data[1] == 0xFF;   // check protocol version and its inverse
    }

    static std::optional<DoIPPayloadType> getPayloadType(const uint8_t *data, size_t length) {
        if (data == nullptr || length < DoIPMessageHeader::DOIP_HEADER_SIZE) {
            return std::nullopt; // message null or too short
        }

        auto payloadType = toPayloadType(data[2], data[3]);
        if (!payloadType) {
            return std::nullopt; // invalid payload type
        }

        return payloadType;
    }

};

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
     * @brief Constructs a DoIP message with payload type and data.
     *
     * @param pl_type The payload type for this message
     * @param payload The payload data as a vector of bytes
     */
    explicit DoIPMessage(DoIPPayloadType pl_type, const DoIPPayload &payload)
        : m_payload_type(pl_type), m_payload(payload) {}

    /**
     * @brief Constructs a DoIP message with payload type and data (move semantics).
     *
     * @param pl_type The payload type for this message
     * @param payload The payload data as a vector of bytes (moved)
     */
    explicit DoIPMessage(DoIPPayloadType pl_type, DoIPPayload &&payload)
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
     * @param data Pointer to raw payload byte data
     * @param size Number of bytes to copy from payload data
     *
     * @warning No bounds checking is performed. Ensure data points to at least size bytes.
     */
    explicit DoIPMessage(DoIPPayloadType pl_type, const uint8_t *data, size_t size)
        : m_payload_type(pl_type), m_payload(data, data + size) {}

    /**
     * @brief Constructs a DoIP message from iterator range.
     *
     * @tparam Iterator Input iterator type
     * @param pl_type The payload type for this message
     * @param first Iterator to the beginning of the data range
     * @param last Iterator to the end of the data range
     */
    template <typename Iterator>
    explicit DoIPMessage(DoIPPayloadType pl_type, Iterator first, Iterator last)
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
    const DoIPPayload &getPayload() const { return m_payload; }

    /**
     * @brief Sets the payload data.
     *
     * @param payload The new payload data
     */
    void setPayload(const DoIPPayload &payload) { m_payload = payload; }

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
    size_t getMessageSize() const { return getPayloadSize() + DoIPMessageHeader::DOIP_HEADER_SIZE; }

    /**
     * @brief Get the Source Address of the message (if message is a Diagnostic Message).
     *
     * @return std::optional<DoIPAddress> The source address if present, std::nullopt otherwise
     */
    std::optional<DoIPAddress> getSourceAddress() const {
        if (m_payload_type == DoIPPayloadType::DiagnosticMessage && m_payload.size() >= 2) {
            // TODO: validate source address range
            // if (!DoIPAddress::isValidSourceAddress(m_payload.data(), 0)) {
            //     std::cerr << "Invalid source address in Diagnostic Message\n";
            //     return std::nullopt;
            // }
            return DoIPAddress(m_payload.data(), 0);
        }
        return std::nullopt;
    }

    /**
     * @brief Get the Target Address of the message (if message is a Diagnostic Message).
     *
     * @return std::optional<DoIPAddress> The target address if present, std::nullopt otherwise
     */
    std::optional<DoIPAddress> getTargetAddress() const {
        if (m_payload_type == DoIPPayloadType::DiagnosticMessage && m_payload.size() >= 4) {
            return DoIPAddress(m_payload.data(), 2);
        }
        return std::nullopt;
    }

    /**
     * @brief Get the Diagnostic Message Payload (if message is a Diagnostic Message).
     *
     * @return std::optional<std::pair<const uint8_t*, size_t>> Pointer to payload data and size if present, std::nullopt otherwise
     */
    std::optional<std::pair<const uint8_t*, size_t>> getDiagnosticMessagePayload() const {
        if (m_payload_type == DoIPPayloadType::DiagnosticMessage && m_payload.size() > 4) {
            return std::make_pair(m_payload.data() + 4, m_payload.size() - 4);
        }
        return std::nullopt;
    }

    void appendPayloadType(ByteArray &bytes) const {
        uint16_t plt = static_cast<uint16_t>(m_payload_type);
        bytes.emplace_back(plt >> 8 & 0xff);
        bytes.emplace_back(plt & 0xff);
    }


    static std::optional<DoIPMessage> fromRaw(const uint8_t *data, size_t length) {
        auto optHeader = DoIPMessageHeader::parseHeader(data, length);
        if (!optHeader.has_value()) {
            return std::nullopt;
        }

        auto [payloadType, payloadLength] = optHeader.value();
        auto expectedLength = DoIPMessageHeader::DOIP_HEADER_SIZE + payloadLength;

        if (expectedLength != length) {
            std::cerr << "Payload length mismatch: Exp " << expectedLength  << ", got " << length << '\n';
            return std::nullopt; // inconsistent payload size
        }

        return DoIPMessage(payloadType, data + DoIPMessageHeader::DOIP_HEADER_SIZE, length - DoIPMessageHeader::DOIP_HEADER_SIZE);
    }

    /**
     * @brief Creates a vehicle identification request message.
     *
     * @return DoIPMessage the DoIP message
     */
    static DoIPMessage makeVehicleIdentificationRequest() {
        return DoIPMessage(DoIPPayloadType::VehicleIdentificationRequest, {});
    }

    /**
     * @brief Creates a vehicle identification response message.
     *
     * @param vin the vehicle identification number (VIN)
     * @param logicalAddress the logical address of the entity
     * @param entityType the entity identifier (EID)
     * @param groupId the group identifier (GID)
     * @param furtherAction the further action code
     * @param syncStatus the synchronization status
     * @return DoIPMessage the vehicle identification response message
     */
    static DoIPMessage makeVehicleIdentificationResponse(const DoIPVIN& vin, const DoIPAddress& logicalAddress, const DoIPEID& entityType, const DoIPGID& groupId, DoIPFurtherAction furtherAction = DoIPFurtherAction::NoFurtherAction, DoIPSyncStatus syncStatus = DoIPSyncStatus::GidVinSynchronized) {
        // Table 5, p. 21
        ByteArray payload;

        vin.appendTo(payload);
        logicalAddress.appendTo(payload);
        entityType.appendTo(payload);
        groupId.appendTo(payload);
        payload.emplace_back(static_cast<uint8_t>(furtherAction));
        payload.emplace_back(static_cast<uint8_t>(syncStatus));

        return DoIPMessage(DoIPPayloadType::VehicleIdentificationResponse, payload);
    }


    /**
     * @brief Creates a generic DoIP negative response (NACK).
     *
     * @param nack the negative response code,
     * @return DoIPMessage the DoIP message
     */
    static DoIPMessage makeNegativeAckMessage(DoIPNegativeAck nack) {
        return DoIPMessage(DoIPPayloadType::NegativeAck, {static_cast<uint8_t>(nack)});
    }

    /**
     * @brief Creates a diagnostic message.
     * @note Table 21
     *
     * @param sa the source address
     * @param ta the target address
     * @param msg_payload the original diagnostic messages (e. g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    static DoIPMessage makeDiagnosticMessage(const DoIPAddress &sa, const DoIPAddress &ta, const ByteArray &msg_payload) {
        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size());

        sa.appendTo(payload);
        ta.appendTo(payload);

        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessage, payload);
    }

    /**
     * @brief Creates a diagnostic positive ACK message.
     * @note Table 23
     * @param sa the source address
     * @param ta the target address
     * @param msg_payload the original diagnostic messages (e. g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    static DoIPMessage makeDiagnosticPositiveResponse(const DoIPAddress &sa, const DoIPAddress &ta, const ByteArray &msg_payload) {
        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size() + 1);

        sa.appendTo(payload);
        ta.appendTo(payload);

        payload.emplace_back(DIAGNOSTIC_MESSAGE_ACK);

        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessageAck, payload);
    }

    /**
     * @brief Creates a diagnostic negative ACK message (NACK).
     * @note Table 23
     * @param sa the source address
     * @param ta the target address
     * @param msg_payload the original diagnostic messages (e. g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    static DoIPMessage makeDiagnosticNegativeResponse(const DoIPAddress &sa, const DoIPAddress &ta, DoIPNegativeDiagnosticAck nack, const ByteArray &msg_payload) {
        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size() + 1);

        sa.appendTo(payload);
        ta.appendTo(payload);

        payload.emplace_back(static_cast<uint8_t>(nack));

        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessageNegativeAck, payload);
    }

    /**
     * @brief Create a 'alive check' request
     * @note Table 27
     * @return DoIPMessage
     */
    static DoIPMessage makeAliveCheckRequest() {
        return DoIPMessage(DoIPPayloadType::AliveCheckRequest, {});
    }

    /**
     * @brief Create a 'alive check' request
     * @note Table 28
     * @param sa the source address
     * @return DoIPMessage
     */
    static DoIPMessage makeAliveCheckResponse(const DoIPAddress &sa) {
        ByteArray payload;
        payload.reserve(sa.size());
        sa.appendTo(payload);

        return DoIPMessage(DoIPPayloadType::AliveCheckResponse, payload);
    }

    /**
     * @brief Creates a routing activation response message.
     *
     * @param routingReq the routing activation request message
     * @param ea the entity (our) address
     * @param actType the activation type
     * @return DoIPMessage the activation response message
     */
    static DoIPMessage makeRoutingRequest(const DoIPAddress &ea, DoIPActivationType actType = DoIPActivationType::Default) {
        ByteArray payload;
        payload.reserve(ea.size() + 1 + 4);
        ea.appendTo(payload);
        payload.emplace_back(static_cast<uint8_t>(actType));
        // Reserved 4 bytes for further use, p. 67
        payload.insert(payload.end(), {0, 0, 0, 0});

        return DoIPMessage(DoIPPayloadType::RoutingActivationRequest, payload);
    }

    static DoIPMessage makeRoutingResponse(const DoIPMessage &routingReq, const DoIPAddress &ea, DoIPActivationType actType = DoIPActivationType::Default) {
        ByteArray payload;
        payload.reserve(ea.size() + 1 + 4);
        routingReq.getSourceAddress().value().appendTo(payload);
        ea.appendTo(payload);
        payload.emplace_back(static_cast<uint8_t>(actType));
        // Reserved 4 bytes for further use, p. 67
        payload.insert(payload.end(), {0, 0, 0, 0});

        return DoIPMessage(DoIPPayloadType::RoutingActivationRequest, payload);
    }

    /**
     * @brief Gets the complete DoIP messages as byte array.
     *
     * @return ByteArray the DoIP message bytes
     */
    [[nodiscard]] ByteArray toByteArray() const {
        ByteArray bytes;
        bytes.reserve(getMessageSize()); // Pre-allocate memory but keep size = 0

        bytes.emplace_back(PROTOCOL_VERSION);
        bytes.emplace_back(PROTOCOL_VERSION_INV);

        appendPayloadType(bytes);
        appendSize(bytes);
        bytes.insert(bytes.end(), m_payload.begin(), m_payload.end());

        return bytes;
    }

    // TODO: Periodic message, p. 49
    // TODO: Entity status request/response, p. 25

  protected:
    DoIPPayloadType m_payload_type; ///< The type of DoIP payload
    DoIPPayload m_payload;          ///< The payload data as byte vector

    /**
     * @brief Appends the address to the payload.
     *
     * @param address the address to append
     */
    // cppcheck-suppress unusedFunction
    void appendAddress(const DoIPAddress &address) {
        m_payload.emplace_back(address.hsb());
        m_payload.emplace_back(address.lsb());
    }
    // cppcheck-suppress-end unusedFunction

    void appendSize(ByteArray &bytes) const {
        uint32_t size = static_cast<uint32_t>(m_payload.size());

        bytes.emplace_back(size >> 24 & 0xff);
        bytes.emplace_back(size >> 16 & 0xff);
        bytes.emplace_back(size >> 8 & 0xff);
        bytes.emplace_back(size & 0xff);
    }
};

/**
 * @brief Stream operator for DoIPMessage
 *
 * Prints the protocol version, payload type, payload size, and payload data.
 * Example output:
 * "DoIPMessage [Protocol: 0x04, Type: DiagnosticMessage (0x8001), Size: 5 bytes, Payload: 00.01.00.3E.01]"
 *
 * @param os Output stream
 * @param msg DoIPMessage to print
 * @return std::ostream& Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const DoIPMessage& msg) {
    os << "V"
       << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<unsigned int>(PROTOCOL_VERSION) << std::dec
       << "|" << msg.getPayloadType()
       << "|L" << msg.getPayloadSize() << " bytes"
       << "| " << msg.getPayload() << "]";

    return os;
}

} // namespace doip

#endif /* DOIPMESSAGE_H */

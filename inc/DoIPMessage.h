#ifndef DOIPMESSAGE_IMPROVED_H
#define DOIPMESSAGE_IMPROVED_H

#include <optional>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <memory>

#include "AnsiColors.h"
#include "ByteArray.h"
#include "DoIPAddress.h"
#include "DoIPRoutingActivationType.h"
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
 */
constexpr uint8_t DIAGNOSTIC_MESSAGE_ACK = 0;

/**
 * @brief Size of the DoIP header
 */
constexpr size_t DOIP_HEADER_SIZE = 8;

/**
 * @brief Size of the DoIP diagnostic message header
 */
constexpr size_t DOIP_DIAG_HEADER_SIZE = DOIP_HEADER_SIZE + 4;

class DoIPMessage;
using OptDoIPMessage = std::optional<DoIPMessage>;


/**
 * @brief Represents a complete DoIP message with internal ByteArray representation.
 *
 * The message is stored internally as a complete ByteArray including the 8-byte header
 * and payload. This eliminates the need for copying when sending messages.
 *
 * Memory layout:
 * [0]      Protocol Version
 * [1]      Inverse Protocol Version
 * [2-3]    Payload Type (big-endian uint16_t)
 * [4-7]    Payload Length (big-endian uint32_t)
 * [8...]   Payload data
 *
 * The message types are listed in table 17.
 */
class DoIPMessage {


public:
    /**
     * @brief Default constructor - creates invalid empty message
     */
    DoIPMessage() : m_data() {}

    /**
     * @brief Constructs a DoIP message with payload type and data.
     *
     * @param payloadType The payload type for this message
     * @param payload The payload data as a vector of bytes
     */
    explicit DoIPMessage(DoIPPayloadType payloadType, const ByteArray &payload) {
        buildMessage(payloadType, payload);
    }

    /**
     * @brief Constructs a DoIP message with payload type and data (move semantics).
     *
     * @param payloadType The payload type for this message
     * @param payload The payload data as a vector of bytes (moved)
     */
    explicit DoIPMessage(DoIPPayloadType payloadType, ByteArray &&payload) {
        buildMessage(payloadType, payload);
    }

    /**
     * @brief Constructs a DoIP message with payload type and initializer list.
     *
     * @param payloadType The payload type for this message
     * @param init_list Initializer list of bytes for the payload
     */
    DoIPMessage(DoIPPayloadType payloadType, std::initializer_list<uint8_t> init_list) {
        ByteArray payload(init_list);
        buildMessage(payloadType, payload);
    }

    /**
     * @brief Constructs a DoIP message from raw byte array.
     *
     * @param payloadType The payload type for this message
     * @param data Pointer to raw payload byte data
     * @param size Number of bytes to copy from payload data
     */
    explicit DoIPMessage(DoIPPayloadType payloadType, const uint8_t *data, size_t size) {
        ByteArray payload(data, size);
        buildMessage(payloadType, payload);
    }

    /**
     * @brief Constructs a DoIP message from iterator range.
     *
     * @tparam Iterator Input iterator type
     * @param payloadType The payload type for this message
     * @param first Iterator to the beginning of the data range
     * @param last Iterator to the end of the data range
     */
    template <typename Iterator>
    explicit DoIPMessage(DoIPPayloadType payloadType, Iterator first, Iterator last) {
        ByteArray payload(first, last);
        buildMessage(payloadType, payload);
    }

    /**
     * @brief Gets the payload type of this message.
     *
     * @return The DoIP payload type
     */
    DoIPPayloadType getPayloadType() const {
        if (m_data.size() < DOIP_HEADER_SIZE) {
            return DoIPPayloadType::NegativeAck; // or throw exception
        }
        uint16_t type_value = (static_cast<uint16_t>(m_data[2]) << 8) | m_data[3];
        return static_cast<DoIPPayloadType>(type_value);
    }

    /**
     * @brief Gets the payload data (without header).
     *
     * @return ByteArrayRef pointing to the payload portion
     */
    ByteArrayRef getPayload() const {
        if (m_data.size() <= DOIP_HEADER_SIZE) {
            return {nullptr, 0};
        }
        return {m_data.data() + DOIP_HEADER_SIZE, m_data.size() - DOIP_HEADER_SIZE};
    }

    /**
     * @brief Gets the payload data (without header).
     *
     * @return ByteArrayRef pointing to the payload portion
     */
    ByteArrayRef getDiagnosticMessagePayload() const {
        if (m_data.size() <= DOIP_DIAG_HEADER_SIZE) {
            return {nullptr, 0};
        }
        return {m_data.data() + DOIP_DIAG_HEADER_SIZE, m_data.size() - DOIP_DIAG_HEADER_SIZE};
    }

    /**
     * @brief Gets the payload size in bytes (without header).
     *
     * @return size_t Number of bytes in the payload
     */
    size_t getPayloadSize() const {
        if (m_data.size() < DOIP_HEADER_SIZE) {
            return 0;
        }
        return m_data.size() - DOIP_HEADER_SIZE;
    }

    /**
     * @brief Get the complete message size (header + payload).
     *
     * @return size_t Total number of bytes
     */
    size_t getMessageSize() const {
        return m_data.size();
    }

    /**
     * @brief Gets direct access to the complete message data for sending.
     *
     * This method returns a pointer and size without copying, making it
     * efficient for network transmission.
     *
     * @return ByteArrayRef Reference to the complete message (header + payload)
     */
    ByteArrayRef getData() const {
        return {m_data.data(), m_data.size()};
    }

    /**
     * @brief Gets direct pointer to the message data (for legacy APIs).
     *
     * @return const uint8_t* Pointer to the message data
     */
    const uint8_t* data() const {
        return m_data.data();
    }

    /**
     * @brief Gets the size for use with legacy APIs.
     *
     * @return size_t Size of the complete message
     */
    size_t size() const {
        return m_data.size();
    }

    /**
     * @brief Get the complete message as ByteArray.
     *
     * @return const ByteArray& Reference to the complete message data
     */
    const ByteArray& asByteArray() const {
        return m_data;
    }

    /**
     * @brief Get a copy of the complete message as ByteArray.
     *
     * @return ByteArray A copy of the complete message data
     */
    ByteArray copyAsByteArray() const {
        return m_data;
    }

    /**
     * @brief Get the Source Address of the message (if message is a Diagnostic Message).
     *
     * @return std::optional<DoIPAddress> The source address if present, std::nullopt otherwise
     */
    std::optional<DoIPAddress> getSourceAddress() const {
        auto payloadRef = getPayload();
        if (getPayloadType() == DoIPPayloadType::DiagnosticMessage && payloadRef.second >= 2) {
            return DoIPAddress(payloadRef.first, 0);
        }
        if (getPayloadType() == DoIPPayloadType::RoutingActivationRequest && payloadRef.second >= 2) {
            return DoIPAddress(payloadRef.first, 0);
        }
        return std::nullopt;
    }

    /**
     * @brief Get the Target Address of the message (if message is a Diagnostic Message).
     *
     * @return std::optional<DoIPAddress> The target address if present, std::nullopt otherwise
     */
    std::optional<DoIPAddress> getTargetAddress() const {
        auto payloadRef = getPayload();
        if (getPayloadType() == DoIPPayloadType::DiagnosticMessage && payloadRef.second >= 4) {
            return DoIPAddress(payloadRef.first, 2);
        }
        return std::nullopt;
    }

    /**
     * @brief Checks if the message is valid.
     *
     * @return bool True if the message has a valid structure
     */
    bool isValid() const {
        return m_data.size() >= DOIP_HEADER_SIZE &&
               isValidProtocolVersion() &&
               getPayloadSize() == getPayloadLengthFromHeader();
    }

        /**
     * @brief Checks if the protocol version is valid.
     * @param data Pointer to the byte array
     * @param length Length of the data
     * @param offset Offset to start checking from (default: 0)
     * @return bool True if valid
     */
    static bool isValidProtocolVersion(const uint8_t *data, size_t length, size_t offset = 0)  {
        if (length < 2) {
            return false;
        }

        uint8_t version = data[offset];
        uint8_t version_inv = data[offset + 1];
        // Check protocol version and its inverse
        return version >= ISO_13400_2010 &&
               version <= ISO_13400_2025 &&
               version + version_inv == 0xFF;
    }

    /**
     * @brief Gets the payload type from raw data.
     *
     * @param data Pointer to the byte array
     * @param length Length of the data
     * @return std::optional<DoIPPayloadType> The payload type if valid
     */
    static std::optional<std::pair<DoIPPayloadType, uint32_t>> tryParseHeader(const uint8_t *data, size_t length) {
        if (!data || length < DOIP_HEADER_SIZE) return std::nullopt;

        if (!isValidProtocolVersion(data, length)) {
            return std::nullopt;
        }

        auto payloadType = toPayloadType(data[2], data[3]);
        if (!payloadType) {
            return std::nullopt; // invalid payload type
        }

        // Extract payload length from header
        uint32_t payloadLength = util::readU32BE(data, 4);

        return std::make_pair(payloadType.value(), payloadLength);
    }

    /**
     * @brief Parse a DoIP message from raw data.
     *
     * @param data Pointer to raw message data
     * @param length Length of the data
     * @return std::optional<DoIPMessage> The parsed message or std::nullopt if invalid
     */
    static std::optional<DoIPMessage> tryParse(const uint8_t *data, size_t length) {
        auto optHeader = tryParseHeader(data, length);
        if (!optHeader.has_value()) {
            return std::nullopt;
        }

        // Check if we have the complete message
        if (length < DOIP_HEADER_SIZE + optHeader->second) {
            return std::nullopt;
        }

        // Create message from complete data
        DoIPMessage msg;
        msg.m_data.assign(data, data + DOIP_HEADER_SIZE + optHeader->second);

        return msg;
    }

protected:
    ByteArray m_data; ///< Complete message data (header + payload)

    /**
     * @brief Builds the internal message representation.
     *
     * @param payloadType The payload type
     * @param payload The payload data
     */
    void buildMessage(DoIPPayloadType payloadType, const ByteArray &payload) {
        m_data.clear();
        m_data.reserve(DOIP_HEADER_SIZE + payload.size());

        // Protocol version
        m_data.emplace_back(PROTOCOL_VERSION);
        m_data.emplace_back(PROTOCOL_VERSION_INV);

        // Payload type (big-endian uint16_t)
        m_data.writeEnum(payloadType);

        // Payload length (big-endian uint32_t)
        uint32_t payloadLength = static_cast<uint32_t>(payload.size());
        m_data.writeU32(payloadLength);

        // Payload data
        m_data.insert(m_data.end(), payload.begin(), payload.end());
    }

    /**
     * @brief Checks if the protocol version is valid.
     *
     * @return bool True if valid
     */
    bool isValidProtocolVersion() const {
        if (m_data.size() < 2) {
            return false;
        }
        // Check protocol version and its inverse
        return m_data[0] >= ISO_13400_2010 &&
               m_data[0] <= ISO_13400_2025 &&
               m_data[0] + m_data[1] == 0xFF;
    }

    /**
     * @brief Gets the payload length from the header.
     *
     * @return size_t The payload length stored in the header
     */
    uint32_t getPayloadLengthFromHeader() const {
        if (m_data.size() < DOIP_HEADER_SIZE) {
            return 0;
        }

        return m_data.readU32BE(4);
    }
};

/**
 * @brief Factory functions for creating specific DoIP message types.
 *
 * These functions provide a clean, type-safe interface for creating
 * different types of DoIP messages without polluting the base class.
 */
namespace message {

    /**
     * @brief Creates a vehicle identification request message.
     *
     * @return DoIPMessage the vehicle identification request
     */
    inline DoIPMessage makeVehicleIdentificationRequest() {
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
    inline DoIPMessage makeVehicleIdentificationResponse(
        const DoIPVIN& vin,
        const DoIPAddress& logicalAddress,
        const DoIPEID& entityType,
        const DoIPGID& groupId,
        DoIPFurtherAction furtherAction = DoIPFurtherAction::NoFurtherAction,
        DoIPSyncStatus syncStatus = DoIPSyncStatus::GidVinSynchronized) {

        ByteArray payload;
        payload.reserve(vin.size() + logicalAddress.size() + entityType.size() + groupId.size() + 2);

        payload.insert(payload.end(), vin.begin(), vin.end());
        payload.writeU16(logicalAddress.toUint16());
        payload.insert(payload.end(), entityType.begin(), entityType.end());
        payload.insert(payload.end(), groupId.begin(), groupId.end());
        payload.writeEnum(furtherAction);
        payload.writeEnum(syncStatus);


        return DoIPMessage(DoIPPayloadType::VehicleIdentificationResponse, std::move(payload));
    }

    /**
     * @brief Creates a generic DoIP negative response (NACK).
     *
     * @param nack the negative response code
     * @return DoIPMessage the DoIP message
     */
    inline DoIPMessage makeNegativeAckMessage(DoIPNegativeAck nack) {
        return DoIPMessage(DoIPPayloadType::NegativeAck, {static_cast<uint8_t>(nack)});
    }

    /**
     * @brief Creates a diagnostic message.
     *
     * @param sa the source address
     * @param ta the target address
     * @param msg_payload the original diagnostic messages (e.g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    inline DoIPMessage makeDiagnosticMessage(
        const DoIPAddress &sa,
        const DoIPAddress &ta,
        const ByteArray &msg_payload) {

        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size());

        sa.appendTo(payload);
        ta.appendTo(payload);
        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessage, std::move(payload));
    }

    /** void b
     * @brief Creates a diagnostic positive ACK message.
     *
     * @param sa the source address
     * @param ta the target address
     * @param msg_payload the original diagnostic messages (e.g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    inline DoIPMessage makeDiagnosticPositiveResponse(
        const DoIPAddress &sa,
        const DoIPAddress &ta,
        const ByteArray &msg_payload) {

        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size() + 1);

        sa.appendTo(payload);
        ta.appendTo(payload);
        payload.emplace_back(DIAGNOSTIC_MESSAGE_ACK);
        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessageAck, std::move(payload));
    }

    /**
     * @brief Creates a diagnostic negative ACK message (NACK).
     *
     * @param sa the source address
     * @param ta the target address
     * @param nack the negative acknowledgment code
     * @param msg_payload the original diagnostic messages (e.g. UDS message)
     * @return DoIPMessage the DoIP message
     */
    inline DoIPMessage makeDiagnosticNegativeResponse(
        const DoIPAddress &sa,
        const DoIPAddress &ta,
        DoIPNegativeDiagnosticAck nack,
        const ByteArray &msg_payload) {

        ByteArray payload;
        payload.reserve(sa.size() + ta.size() + msg_payload.size() + 1);

        sa.appendTo(payload);
        ta.appendTo(payload);
        payload.emplace_back(static_cast<uint8_t>(nack));
        payload.insert(payload.end(), msg_payload.begin(), msg_payload.end());

        return DoIPMessage(DoIPPayloadType::DiagnosticMessageNegativeAck, std::move(payload));
    }

    /**
     * @brief Create an 'alive check' request
     *
     * @return DoIPMessage
     */
    inline DoIPMessage makeAliveCheckRequest() {
        return DoIPMessage(DoIPPayloadType::AliveCheckRequest, {});
    }

    /**
     * @brief Create an 'alive check' response
     *
     * @param sa the source address
     * @return DoIPMessage
     */
    inline DoIPMessage makeAliveCheckResponse(const DoIPAddress &sa) {
        ByteArray payload;
        payload.writeU16(sa.toUint16());
        return DoIPMessage(DoIPPayloadType::AliveCheckResponse, std::move(payload));
    }

    /**
     * @brief Creates a routing activation request message.
     *
     * @param ea the entity address
     * @param actType the activation type
     * @return DoIPMessage the activation request message
     */
    inline DoIPMessage makeRoutingActivationRequest(
        const DoIPAddress &ea,
        DoIPRoutingActivationType actType = DoIPRoutingActivationType::Default) {

        ByteArray payload;
        payload.reserve(ea.size() + 1 + 4);
        payload.writeU16(ea.toUint16());
        payload.writeEnum(actType);
        // Reserved 4 bytes for future use
        payload.insert(payload.end(), {0, 0, 0, 0});

        return DoIPMessage(DoIPPayloadType::RoutingActivationRequest, std::move(payload));
    }

    /**
     * @brief Creates a routing activation response message.
     *
     * @param routingReq the routing request message
     * @param ea the entity address
     * @param actType the activation type
     * @return DoIPMessage the activation response message
     */
    inline DoIPMessage makeRoutingActivationResponse(
        const DoIPMessage &routingReq,
        const DoIPAddress &ea,
        DoIPRoutingActivationType actType = DoIPRoutingActivationType::Default) {

        ByteArray payload;
        payload.reserve(ea.size() + 1 + 4);

        auto sourceAddr = routingReq.getSourceAddress();
        if (sourceAddr) {
            sourceAddr.value().appendTo(payload);
        }

        ea.appendTo(payload);
        payload.emplace_back(static_cast<uint8_t>(actType));
        // Reserved 4 bytes for future use
        payload.insert(payload.end(), {0, 0, 0, 0});

        return DoIPMessage(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    }

} // namespace message

/**
 * @brief Stream operator for DoIPMessage
 *
 * Prints the protocol version, payload type, payload size, and payload data.
 *
 * @param os Output stream
 * @param msg DoIPMessage to print
 * @return std::ostream& Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const DoIPMessage& msg) {
    os << ansi::dim <<
       "V" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<unsigned int>(PROTOCOL_VERSION) << std::dec << ansi::reset
       << "|" << ansi::cyan << msg.getPayloadType() << ansi::reset
       << "|L" << msg.getPayloadSize()
       << "| Payload: ";

    auto payload = msg.getPayload();
    os << ansi::bold_white;
    for (size_t i = 0; i < payload.second; ++i) {
        if (i > 0) os << '.';
        os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(payload.first[i]);
    }
    os << ansi::reset;
    os << std::dec;

    return os;
}

} // namespace doip

#endif /* DOIPMESSAGE_IMPROVED_H */

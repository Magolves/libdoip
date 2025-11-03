#ifndef DOIPGENERICHEADERHANDLER_H
#define DOIPGENERICHEADERHANDLER_H

#include <array>
#include <iomanip>
#include <iostream>
#include <stdint.h>

const int _GenericHeaderLength = 8;
const int _NACKLength = 1;

const uint8_t _IncorrectPatternFormatCode = 0x00;
const uint8_t _UnknownPayloadTypeCode = 0x01;
const uint8_t _InvalidPayloadLengthCode = 0x04;

/**
 * @brief Represents a 16-bit DoIP address consisting of high and low significant bytes.
 *
 * This structure encapsulates a 16-bit address used in DoIP (Diagnostic over Internet Protocol)
 * communication. The address is stored as two separate bytes (HSB and LSB) and provides
 * convenient methods for construction, access, and manipulation.
 *
 * @note The address follows big-endian byte ordering (HSB first, then LSB).
 */
struct Address {
    /**
     * @brief Constructs an Address with specified high and low significant bytes.
     *
     * @param hsb High significant byte (default: 0)
     * @param lsb Low significant byte (default: 0)
     */
    Address(uint8_t hsb = 0, uint8_t lsb = 0) : m_bytes{{hsb, lsb}} {}

    /**
     * @brief Constructs an Address from a byte array starting at the specified offset.
     *
     * This constructor reads two consecutive bytes from the provided data array,
     * starting at the given offset. The first byte becomes the HSB and the second
     * byte becomes the LSB.
     *
     * @param data Pointer to the byte array containing address data
     * @param offset Starting offset in the data array (default: 0)
     *
     * @note If data is nullptr, the address is initialized to {0, 0}
     * @warning No bounds checking is performed on the data array
     */
    Address(const uint8_t *data, size_t offset = 0) : m_bytes{0, 0} {
        if (data != nullptr) {
            m_bytes[HSB] = data[offset];
            m_bytes[LSB] = data[offset + 1];
        }
    }

    /**
     * @brief Gets the low significant byte of the address.
     *
     * @return The low significant byte (LSB) as uint8_t
     */
    uint8_t lsb() const { return m_bytes[LSB]; }

    /**
     * @brief Gets the high significant byte of the address.
     *
     * @return The high significant byte (HSB) as uint8_t
     */
    uint8_t hsb() const { return m_bytes[HSB]; }

    /**
     * @brief Converts the address to a 16-bit unsigned integer.
     *
     * The conversion follows big-endian byte ordering where the HSB forms
     * the upper 8 bits and the LSB forms the lower 8 bits.
     *
     * @return The address as a 16-bit unsigned integer in host byte order
     */
    uint16_t as_uint16() const { return (m_bytes[0] << 8) | m_bytes[1]; }

    /**
     * @brief Gets a pointer to the internal byte array.
     *
     * This method provides direct access to the underlying byte storage.
     * The returned pointer points to a 2-byte array where:
     * - data()[0] contains the HSB
     * - data()[1] contains the LSB
     *
     * @return Const pointer to the internal 2-byte array
     */
    const uint8_t *data() const { return m_bytes.data(); }

    /**
     * @brief Updates the address with new high and low significant bytes.
     *
     * @param hsb New high significant byte
     * @param lsb New low significant byte
     */
    void update(uint8_t hsb, uint8_t lsb) {
        m_bytes[HSB] = hsb;
        m_bytes[LSB] = lsb;
    }

    /**
     * @brief Updates the address from a byte array starting at the specified offset.
     *
     * This method reads two consecutive bytes from the provided data array,
     * starting at the given offset. The first byte becomes the HSB and the second
     * byte becomes the LSB, overwriting the current address values.
     *
     * @param data Pointer to the byte array containing new address data
     * @param offset Starting offset in the data array (default: 0)
     *
     * @warning No bounds checking is performed on the data array. Ensure that
     *          data[offset] and data[offset+1] are valid memory locations.
     * @warning No null pointer checking is performed. Ensure data is not nullptr.
     */
    void update(const uint8_t* data, size_t offset = 0) {
        m_bytes[HSB] = data[offset];
        m_bytes[LSB] = data[offset + 1];
    }

    /**
     * @brief Compares two Address objects for equality.
     *
     * Two addresses are considered equal if both their high significant bytes
     * and low significant bytes are identical.
     *
     * @param other The Address object to compare with
     * @return true if both addresses are equal, false otherwise
     */
    bool operator==(const Address& other) const {
        return as_uint16() == other.as_uint16();
    }

    /**
     * @brief Compares two Address objects for inequality.
     *
     * Two addresses are considered unequal if either their high significant bytes
     * or low significant bytes differ.
     *
     * @param other The Address object to compare with
     * @return true if the addresses are not equal, false otherwise
     */
    bool operator!=(const Address& other) const {
        return !(*this == other);
    }

    /**
     * @brief Check if source address is valid
     *
     * @return true the source address is valid
     * @return false the source address is NOT valid
     */
    bool isValidSourceAddress() const {
        return minSourceAddress <= as_uint16() && maxSourceAddress >= as_uint16();
    }

    static constexpr uint16_t minSourceAddress = 3584; // 0x0E00
    static constexpr uint16_t maxSourceAddress = 4095; // 0x0FFF

  private:
    static constexpr uint8_t HSB = 0; ///< Index for High Significant Byte in the byte array
    static constexpr uint8_t LSB = 1; ///< Index for Low Significant Byte in the byte array

    std::array<uint8_t, 2> m_bytes; ///< Internal storage for the 2-byte address
};

/**
 * @brief Stream output operator for Address objects.
 *
 * Outputs the address in a human-readable hexadecimal format.
 * The format is "0xHHHH" where HHHH is the 4-digit hexadecimal representation
 * of the 16-bit address value.
 *
 * @param os The output stream to write to
 * @param addr The Address object to output
 * @return Reference to the output stream for chaining
 */
inline std::ostream& operator<<(std::ostream& os, const Address& addr) {
    os << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addr.as_uint16();
    return os;
}

enum PayloadType : uint8_t {
    NEGATIVEACK,
    ROUTINGACTIVATIONREQUEST,
    ROUTINGACTIVATIONRESPONSE,
    VEHICLEIDENTREQUEST,
    VEHICLEIDENTRESPONSE,
    DIAGNOSTICMESSAGE,
    DIAGNOSTICPOSITIVEACK,
    DIAGNOSTICNEGATIVEACK,
    ALIVECHECKRESPONSE,
};

struct GenericHeaderAction {
    PayloadType type;
    uint8_t value;
    unsigned long payloadLength;
};

GenericHeaderAction parseGenericHeader(uint8_t *data, int dataLenght);
uint8_t *createGenericHeader(PayloadType type, uint32_t length);

#endif /* DOIPGENERICHEADERHANDLER_H */

#ifndef DOIPADDRESS_H
#define DOIPADDRESS_H

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <iomanip>
#include <iostream>

#include "ByteArray.h"

namespace doip {

constexpr size_t DOIP_ADDRESS_SIZE = 2;

/**
 * @brief Represents a 16-bit DoIP address consisting of high and low significant bytes.
 *
 * This structure encapsulates a 16-bit address used in DoIP (Diagnostic over Internet Protocol)
 * communication. The address is stored as two separate bytes (HSB and LSB) and provides
 * convenient methods for construction, access, and manipulation.
 *
 * @note The address follows big-endian byte ordering (HSB first, then LSB).
 */

struct DoIPAddress {
    /**
     * @brief Constructs an DoIPAddress with specified high and low significant bytes.
     *
     * @param hsb High significant byte (default: 0)
     * @param lsb Low significant byte (default: 0)
     */
    constexpr explicit DoIPAddress(uint8_t hsb = 0, uint8_t lsb = 0) : m_bytes{{hsb, lsb}} {}

    /**
     * @brief Constructs an DoIPAddress from a byte array starting at the specified offset.
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
    constexpr explicit DoIPAddress(const uint8_t *data, size_t offset = 0) : m_bytes{0, 0} {
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
     * @brief Gets the high significant byteconst DoIPAddress& sa of the address.
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
    uint16_t toUint16() const { return (m_bytes[0] << 8) | m_bytes[1]; }

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
     * @brief Size of the address in bytes.
     *
     * @return constexpr size_t the address size
     */
    constexpr size_t size() const { return DOIP_ADDRESS_SIZE; }

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

    void update(uint16_t hsblsb) {
        update((hsblsb >> 8) & 0xFF, hsblsb & 0xFF);
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
    void update(const uint8_t *data, size_t offset = 0) {
        if (data == nullptr) {
            return;
        }
        update(data[offset], data[offset + 1]);
    }

    /**
     * @brief Appends this address to the given byte array.
     *
     * @param bytes the byte array to append to
     * @return ByteArray& the modified byte array
     */
    ByteArray &appendTo(ByteArray &bytes) const {
        bytes.emplace_back(hsb());
        bytes.emplace_back(lsb());
        return bytes;
    }

    /**
     * @brief Compares two DoIPAddress objects for equality.
     *
     * Two addresses are considered equal if both their high significant bytes
     * and low significant bytes are identical.
     *
     * @param other The DoIPAddress object to compare with
     * @return true if both addresses are equal, false otherwise
     */
    bool operator==(const DoIPAddress &other) const {
        return toUint16() == other.toUint16();
    }

    /**
     * @brief Compares two DoIPAddress objects for inequality.
     *
     * Two addresses are considered unequal if either their high significant bytes
     * or low significant bytes differ.
     *
     * @param other The DoIPAddress object to compare with
     * @return true if the addresses are not equal, false otherwise
     */
    bool operator!=(const DoIPAddress &other) const {
        return !(*this == other);
    }

    /**
     * @brief Check if source address is valid
     *
     * @return true the source address is valid
     * @return false the source address is NOT valid
     */
    bool isValidSourceAddress() {
        return isValidSourceAddress(m_bytes.data());
    }

    /**
     * @brief Check if source address is valid
     * @param data the data array containing the address
     * @param offset the offset in the data array where the address starts
     * @return true the source address is valid
     * @return false the source address is NOT valid
     */
    static bool isValidSourceAddress(const uint8_t *data, size_t offset = 0) {
        uint16_t addr_value = (data[offset] << 8) | data[offset + 1];

        return MIN_SOURCE_ADDRESS <= addr_value && MAX_SOURCE_ADDRESS >= addr_value;
    }

    // ?? Nothing found in ISO 13400-2 ??
    static constexpr uint16_t MIN_SOURCE_ADDRESS = 3584; // 0x0E00
    static constexpr uint16_t MAX_SOURCE_ADDRESS = 4095; // 0x0FFF

    /**
     * @brief Constant for a DoIP zero address (0x0000).
     */
    static const DoIPAddress ZeroAddress;

  private:
    static constexpr uint8_t HSB = 0; ///< Index for High Significant Byte in the byte array
    static constexpr uint8_t LSB = 1; ///< Index for Low Significant Byte in the byte array

    std::array<uint8_t, DOIP_ADDRESS_SIZE> m_bytes; ///< Internal storage for the 2-byte address
};

/**
 * @brief Stream output operator for DoIPAddress objects.
 *
 * Outputs the address in a human-readable hexadecimal format.
 * The format is "0xHHHH" where HHHH is the 4-digit hexadecimal representation
 * of the 16-bit address value.
 *
 * @param os The output stream to write to
 * @param addr The DoIPAddress object to output
 * @return Reference to the output stream for chaining
 */
inline std::ostream &operator<<(std::ostream &os, const DoIPAddress &addr) {
    os << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addr.toUint16();
    return os;
}

inline const DoIPAddress DoIPAddress::ZeroAddress = DoIPAddress();

} // namespace doip

#endif /* DOIPADDRESS_H */

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
using DoIPAddress = uint16_t;

constexpr DoIPAddress ZERO_ADDRESS = 0x0000;
constexpr DoIPAddress MIN_SOURCE_ADDRESS = 0xE000;
constexpr DoIPAddress MAX_SOURCE_ADDRESS = 0xE3FF;

/**
 * @brief Check if source address is valid.
 * @param data the data array containing the address
 * @param offset the offset in the data array where the address starts
 * @return true the source address is valid
 * @return false the source address is NOT valid
 */
inline bool isValidSourceAddress(const uint8_t *data, size_t offset = 0) {
    uint16_t addr_value = (data[offset] << 8) | data[offset + 1];

    return MIN_SOURCE_ADDRESS <= addr_value && MAX_SOURCE_ADDRESS >= addr_value;
}

/**
 * @brief Try read the DoIP address from a byte array.
 * @note No bounds checking
 * @param data the pointer to the data array
 * @param offset the offset in bytes
 * @param address the address read
 * @return true address was read successfully
 * @return false invalid arguments
 */
inline bool tryReadAddressFrom(const uint8_t *data, size_t offset, DoIPAddress &address) {
    if (!data)
        return false;

    address = static_cast<uint16_t>(data[offset] << 8 | data[offset + 1]);
    return true;
}

/**
 * @brief Reads the DoIP address from a byte array.
 * @note No bounds checking
 * @param data the pointer to the data array
 * @param offset the offset in bytes
 * @return DoIPAddress the parsed DoIP address. If data is nullptr, then ZERO_ADDRESS is returned.
 */
inline DoIPAddress readAddressFrom(const uint8_t *data, size_t offset = 0) {
    if (!data)
        return ZERO_ADDRESS;

    return static_cast<uint16_t>(data[offset] << 8 | data[offset + 1]);
}

} // namespace doip

#endif /* DOIPADDRESS_H */

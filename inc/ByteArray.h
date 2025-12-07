/**
 * @file ByteArray.h
 * @brief Defines the ByteArray type and utility functions for byte manipulation
 *
 * This file provides a ByteArray type (based on std::vector<uint8_t>) with
 * convenient methods for reading and writing multi-byte integer values in
 * big-endian format, commonly used in network protocols.
 */

#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <iomanip>
#include <ostream>
#include <stdint.h>
#include <type_traits>
#include <vector>

namespace doip {

namespace util {

/**
 * @brief Reads a 16-bit unsigned integer in big-endian format from a byte array
 *
 * @param data Pointer to the byte array
 * @param index Starting index in the array
 * @return uint16_t The 16-bit value read from the array
 */
inline uint16_t readU16BE(const uint8_t *data, size_t index) {
    return (static_cast<uint16_t>(data[index]) << 8) |
           static_cast<uint16_t>(data[index + 1]);
}

/**
 * @brief Reads a 32-bit unsigned integer in big-endian format from a byte array
 *
 * @param data Pointer to the byte array
 * @param index Starting index in the array
 * @return uint32_t The 32-bit value read from the array
 */
inline uint32_t readU32BE(const uint8_t *data, size_t index) {
    return (static_cast<uint32_t>(data[index]) << 24) |
           (static_cast<uint32_t>(data[index + 1]) << 16) |
           (static_cast<uint32_t>(data[index + 2]) << 8) |
           static_cast<uint32_t>(data[index + 3]);
}

} // namespace util

/**
 * @brief A dynamic array of bytes with utility methods for network protocol handling
 *
 * ByteArray extends std::vector<uint8_t> with convenient methods for reading
 * and writing multi-byte integer values in big-endian format (network byte order).
 * This is commonly used in DoIP and other network protocols.
 *
 * @note All multi-byte read/write operations use big-endian (network) byte order
 */
struct ByteArray : std::vector<uint8_t> {
    /**
     * @brief Default constructor - creates an empty ByteArray
     */
    ByteArray() = default;

    /**
     * @brief Constructs a ByteArray from a raw byte array
     *
     * @param data Pointer to the source byte array
     * @param size Number of bytes to copy from the source array
     */
    explicit ByteArray(const uint8_t *data, size_t size) : std::vector<uint8_t>(data, data + size) {}

    /**
     * @brief Constructs a ByteArray from an initializer list
     *
     * @param init_list Initializer list of bytes
     *
     * @example
     * ByteArray arr = {0x01, 0x02, 0x03, 0xFF};
     */
    ByteArray(const std::initializer_list<uint8_t> &init_list) : std::vector<uint8_t>(init_list) {}

    /**
     * @brief Writes a 16-bit unsigned integer in big-endian format at a specific index
     *
     * Overwrites 2 bytes starting at the specified index with the value in
     * big-endian (network) byte order.
     *
     * @param index Starting index where to write (must be < size() - 1)
     * @param value The 16-bit value to write
     * @throws std::out_of_range if index + 1 >= size()
     */
    void writeU16At(size_t index, uint16_t value) {
        if (index + 1 >= this->size()) {
            throw std::out_of_range("Index out of range for writeU16BE");
        }
        (*this)[index] = static_cast<uint8_t>((value >> 8) & 0xFF);
        (*this)[index + 1] = static_cast<uint8_t>(value & 0xFF);
    }

    /**
     * @brief Appends a 16-bit unsigned integer in big-endian format to the end
     *
     * Adds 2 bytes to the end of the ByteArray representing the value in
     * big-endian (network) byte order.
     *
     * @param value The 16-bit value to append
     */
    void writeU16BE(uint16_t value) {
        emplace_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        emplace_back(static_cast<uint8_t>(value & 0xFF));
    }

    /**
     * @brief Writes a 32-bit unsigned integer in big-endian format at a specific index
     *
     * Overwrites 4 bytes starting at the specified index with the value in
     * big-endian (network) byte order.
     *
     * @param index Starting index where to write (must be < size() - 3)
     * @param value The 32-bit value to write
     * @throws std::out_of_range if index + 3 >= size()
     */
    void writeU32At(size_t index, uint32_t value) {
        if (index + 3 >= this->size()) {
            throw std::out_of_range("Index out of range for writeU32BE");
        }
        (*this)[index] = static_cast<uint8_t>((value >> 24) & 0xFF);
        (*this)[index + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        (*this)[index + 2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        (*this)[index + 3] = static_cast<uint8_t>(value & 0xFF);
    }

    /**
     * @brief Appends a 32-bit unsigned integer in big-endian format to the end
     *
     * Adds 4 bytes to the end of the ByteArray representing the value in
     * big-endian (network) byte order.
     *
     * @param value The 32-bit value to append
     */
    void writeU32BE(uint32_t value) {
        emplace_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        emplace_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        emplace_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        emplace_back(static_cast<uint8_t>(value & 0xFF));
    }

    /**
     * @brief Appends an enum class value as its underlying integral type
     *
     * Automatically detects the underlying type of the enum and writes it
     * in big-endian format. Supports 8-bit, 16-bit, and 32-bit enums.
     *
     * @tparam E Enum class type
     * @param value The enum value to append
     *
     * @example
     * enum class Status : uint16_t { OK = 0x0100, ERROR = 0x0200 };
     * ByteArray arr;
     * arr.writeEnum(Status::OK); // Appends 0x01, 0x00
     */
    template<typename E>
    void writeEnum(E value) {
        static_assert(std::is_enum_v<E>, "writeEnum requires an enum type");

        using UnderlyingType = std::underlying_type_t<E>;
        auto integral_value = static_cast<UnderlyingType>(value);

        if constexpr (sizeof(UnderlyingType) == 1) {
            emplace_back(static_cast<uint8_t>(integral_value));
        } else if constexpr (sizeof(UnderlyingType) == 2) {
            writeU16BE(static_cast<uint16_t>(integral_value));
        } else if constexpr (sizeof(UnderlyingType) == 4) {
            writeU32BE(static_cast<uint32_t>(integral_value));
        } else {
            static_assert(sizeof(UnderlyingType) <= 4, "Enum underlying type too large (max 32-bit supported)");
        }
    }

    /**
     * @brief Writes an enum class value at a specific index as its underlying integral type
     *
     * Overwrites bytes starting at the specified index with the enum value
     * converted to its underlying integral type in big-endian format.
     *
     * @tparam E Enum class type
     * @param index Starting index where to write
     * @param value The enum value to write
     * @throws std::out_of_range if not enough space at index
     */
    template<typename E>
    void writeEnumAt(size_t index, E value) {
        static_assert(std::is_enum_v<E>, "writeEnumAt requires an enum type");

        using UnderlyingType = std::underlying_type_t<E>;
        auto integral_value = static_cast<UnderlyingType>(value);

        if constexpr (sizeof(UnderlyingType) == 1) {
            if (index >= this->size()) {
                throw std::out_of_range("Index out of range for writeEnumAt");
            }
            (*this)[index] = static_cast<uint8_t>(integral_value);
        } else if constexpr (sizeof(UnderlyingType) == 2) {
            writeU16At(index, static_cast<uint16_t>(integral_value));
        } else if constexpr (sizeof(UnderlyingType) == 4) {
            writeU32At(index, static_cast<uint32_t>(integral_value));
        } else {
            static_assert(sizeof(UnderlyingType) <= 4, "Enum underlying type too large (max 32-bit supported)");
        }
    }

    /**
     * @brief Reads a 16-bit unsigned integer in big-endian format from a specific index
     *
     * Reads 2 bytes starting at the specified index and interprets them as a
     * 16-bit unsigned integer in big-endian (network) byte order.
     *
     * @param index Starting index to read from (must be < size() - 1)
     * @return uint16_t The 16-bit value read from the array
     * @throws std::out_of_range if index + 1 >= size()
     */
    uint16_t readU16BE(size_t index) const {
        if (index + 1 >= this->size()) {
            throw std::out_of_range("Index out of range for readU16BE");
        }
        return util::readU16BE(this->data(), index);
    }

    /**
     * @brief Reads a 32-bit unsigned integer in big-endian format from a specific index
     *
     * Reads 4 bytes starting at the specified index and interprets them as a
     * 32-bit unsigned integer in big-endian (network) byte order.
     *
     * @param index Starting index to read from (must be < size() - 3)
     * @return uint32_t The 32-bit value read from the array
     * @throws std::out_of_range if index + 3 >= size()
     */
    uint32_t readU32BE(size_t index) const {
        if (index + 3 >= this->size()) {
            throw std::out_of_range("Index out of range for readU32BE");
        }
        return util::readU32BE(this->data(), index);
    }

    /**
     * @brief Reads an enum class value from a specific index
     *
     * Reads bytes starting at the specified index and interprets them as
     * the underlying integral type of the enum in big-endian format.
     *
     * @tparam E Enum class type to read
     * @param index Starting index to read from
     * @return E The enum value read from the array
     * @throws std::out_of_range if not enough bytes available
     *
     * @example
     * enum class Status : uint16_t { OK = 0x0100, ERROR = 0x0200 };
     * ByteArray arr = {0x01, 0x00};
     * auto status = arr.readEnum<Status>(0); // Returns Status::OK
     */
    template<typename E>
    E readEnum(size_t index) const {
        static_assert(std::is_enum_v<E>, "readEnum requires an enum type");

        using UnderlyingType = std::underlying_type_t<E>;

        if constexpr (sizeof(UnderlyingType) == 1) {
            if (index >= this->size()) {
                throw std::out_of_range("Index out of range for readEnum");
            }
            return static_cast<E>((*this)[index]);
        } else if constexpr (sizeof(UnderlyingType) == 2) {
            return static_cast<E>(readU16BE(index));
        } else if constexpr (sizeof(UnderlyingType) == 4) {
            return static_cast<E>(readU32BE(index));
        } else {
            static_assert(sizeof(UnderlyingType) <= 4, "Enum underlying type too large (max 32-bit supported)");
        }
    }
};

/**
 * @brief Reference to raw array of bytes.
 *
 * A pair containing a pointer to a byte array and its size.
 * Useful for passing byte array references without copying.
 */
using ByteArrayRef = std::pair<const uint8_t *, size_t>;

/**
 * @brief Stream operator for ByteArray
 *
 * Prints each byte as a two-digit hex value separated by dots.
 * Example: {0x01, 0x02, 0xFF} prints as "01.02.FF"
 *
 * @param os Output stream
 * @param arr ByteArray to print
 * @return std::ostream& Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const ByteArray &arr) {
    std::ios_base::fmtflags flags(os.flags());

    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) {
            os << '.';
        }
        os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(arr[i]);
    }

    os.flags(flags);
    return os;
}

} // namespace doip

#endif /* BYTEARRAY_H */

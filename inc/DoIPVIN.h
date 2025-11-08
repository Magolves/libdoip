#ifndef GenericFixedId_H
#define GenericFixedId_H

#include <array>
#include <string>
#include <cstring>
#include <algorithm>
#include "ByteArray.h"

namespace doip {

/**
 * @brief Represents a Vehicle Identification Number (VIN) with exactly IdLength characters.
 *
 * The VIN is stored as a fixed-size array of IdLength bytes. If the input is shorter than IdLength characters,
 * it is padded with null bytes (0x00). If longer, it is truncated to IdLength characters.
 */
template<size_t IdLength>
class GenericFixedId {
public:
    /**
     * @brief IdLength of a VIN according to ISO standard
     */
    static constexpr size_t VIN_LENGTH = IdLength;

    /**
     * @brief Default constructor - creates a VIN filled with zeros
     */
    GenericFixedId() : m_vin{} {}

    /**
     * @brief Construct from string
     *
     * @param vin_string The VIN as string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const std::string& vin_string) : m_vin{} {
        const size_t copy_length = std::min(vin_string.length(), VIN_LENGTH);
        std::copy_n(vin_string.c_str(), copy_length, m_vin.data());
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from C-style string
     *
     * @param vin_cstr The VIN as C-string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const char* vin_cstr) : m_vin{} {
        if (vin_cstr != nullptr) {
            const size_t copy_length = std::min(std::strlen(vin_cstr), VIN_LENGTH);
            std::copy_n(vin_cstr, copy_length, m_vin.data());
        }
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from byte sequence
     *
     * @param data Pointer to byte data
     * @param length IdLength of the byte data. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    GenericFixedId(const uint8_t* data, size_t length) : m_vin{} {
        if (data != nullptr) {
            const size_t copy_length = std::min(length, VIN_LENGTH);
            std::copy_n(data, copy_length, m_vin.data());
        }
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from ByteArray
     *
     * @param byte_array The VIN as ByteArray. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const ByteArray& byte_array) : m_vin{} {
        const size_t copy_length = std::min(byte_array.size(), VIN_LENGTH);
        std::copy_n(byte_array.data(), copy_length, m_vin.data());
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Copy constructor
     */
    GenericFixedId(const GenericFixedId& other) = default;

    /**
     * @brief Copy assignment operator
     */
    GenericFixedId& operator=(const GenericFixedId& other) = default;

    /**
     * @brief Move constructor
     */
    GenericFixedId(GenericFixedId&& other) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    GenericFixedId& operator=(GenericFixedId&& other) noexcept = default;

    /**
     * @brief Destructor
     */
    ~GenericFixedId() = default;

    /**
     * @brief Get VIN as string
     *
     * @return std::string The VIN as string (may contain null bytes if VIN was shorter than IdLength)
     */
    std::string toString() const {
        // Find the effective length (up to first null byte or VIN_LENGTH)
        size_t effective_length = 0;
        for (size_t i = 0; i < VIN_LENGTH; ++i) {
            if (m_vin[i] == 0) {
                break;
            }
            effective_length = i + 1;
        }
        return std::string(reinterpret_cast<const char*>(m_vin.data()), effective_length);
    }

    /**
     * @brief Get VIN as ByteArray with exactly IdLength bytes
     *
     * @return ByteArray The VIN as byte array (always IdLength bytes)
     */
    ByteArray toByteArray() const {
        return ByteArray(m_vin.begin(), m_vin.end());
    }

    /**
     * @brief Get direct access to the underlying array
     *
     * @return const std::array<uint8_t, VIN_LENGTH>& Reference to the internal array
     */
    const std::array<uint8_t, VIN_LENGTH>& getArray() const {
        return m_vin;
    }

    /**
     * @brief Get pointer to raw data
     *
     * @return const uint8_t* Pointer to the VIN data
     */
    const uint8_t* data() const {
        return m_vin.data();
    }

    /**
     * @brief Get the size (always IdLength)
     *
     * @return constexpr size_t Always returns VIN_LENGTH (IdLength)
     */
    constexpr size_t size() const {
        return m_vin.size();
    }

    /**
     * @brief Check if VIN is all zeros
     *
     * @return bool True if all bytes are zero
     */
    bool isEmpty() const {
        return std::all_of(m_vin.begin(), m_vin.end(), [](uint8_t byte) { return byte == 0; });
    }

    /**
     * @brief Equality operator
     *
     * @param other The other VIN to compare with
     * @return bool True if both VINs are identical
     */
    bool operator==(const GenericFixedId& other) const {
        return m_vin == other.m_vin;
    }

    /**
     * @brief Inequality operator
     *
     * @param other The other VIN to compare with
     * @return bool True if VINs are different
     */
    bool operator!=(const GenericFixedId& other) const {
        return m_vin != other.m_vin;
    }

    /**
     * @brief Array subscript operator (const)
     *
     * @param index Index (0-16)
     * @return const uint8_t& Reference to the byte at the given index
     */
    const uint8_t& operator[](size_t index) const {
        return m_vin.at(index);
    }

    /**
     * @brief Static instance containing only zeros
     */
    static const GenericFixedId VinZero;

private:
    std::array<uint8_t, VIN_LENGTH> m_vin; ///< The VIN data storage
};

// Definition of the static zero VIN for the template
template<size_t IdLength>
inline const GenericFixedId<IdLength> GenericFixedId<IdLength>::VinZero{};

using DoIPVIN = GenericFixedId<17>;

} // namespace doip

#endif /* GenericFixedId_H */


#ifndef DOIP_IDENTIFIERS_H
#define DOIP_IDENTIFIERS_H

#include <array>
#include <string>
#include <cstring>
#include <algorithm>
#include "ByteArray.h"

namespace doip {

/**
 * @brief Generic fixed-length identifier class template.
 *
 * Represents a fixed-length identifier (VIN, EID, GID) with exactly IdLength bytes.
 * The identifier is stored as a fixed-size array. If the input is shorter than IdLength,
 * it is padded with null bytes (0x00). If longer, it is truncated to IdLength.
 */
template<size_t IdLength>
class GenericFixedId {
public:
    /**
     * @brief Length of the identifier in bytes
     */
    static constexpr size_t ID_LENGTH = IdLength;

    /**
     * @brief Default constructor - creates an identifier filled with zeros
     */
    GenericFixedId() : m_id{} {}

    /**
     * @brief Construct from string
     *
     * @param id_string The identifier as string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const std::string& id_string) : m_id{} {
        const size_t copy_length = std::min(id_string.length(), ID_LENGTH);
        std::copy_n(id_string.c_str(), copy_length, m_id.data());
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from C-style string
     *
     * @param id_cstr The identifier as C-string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const char* id_cstr) : m_id{} {
        if (id_cstr != nullptr) {
            const size_t copy_length = std::min(std::strlen(id_cstr), ID_LENGTH);
            std::copy_n(id_cstr, copy_length, m_id.data());
        }
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from byte sequence
     *
     * @param data Pointer to byte data
     * @param length Length of the byte data. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    GenericFixedId(const uint8_t* data, size_t length) : m_id{} {
        if (data != nullptr) {
            const size_t copy_length = std::min(length, ID_LENGTH);
            std::copy_n(data, copy_length, m_id.data());
        }
        // Remaining bytes are already zero-initialized
    }

    /**
     * @brief Construct from ByteArray
     *
     * @param byte_array The identifier as ByteArray. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const ByteArray& byte_array) : m_id{} {
        const size_t copy_length = std::min(byte_array.size(), ID_LENGTH);
        std::copy_n(byte_array.data(), copy_length, m_id.data());
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
     * @brief Get identifier as string
     *
     * @return std::string The identifier as string (may contain null bytes if identifier was shorter than IdLength)
     */
    std::string toString() const {
        // Find the effective length (up to first null byte or ID_LENGTH)
        size_t effective_length = 0;
        for (size_t i = 0; i < ID_LENGTH; ++i) {
            if (m_id[i] == 0) {
                break;
            }
            effective_length = i + 1;
        }
        return std::string(reinterpret_cast<const char*>(m_id.data()), effective_length);
    }

    /**
     * @brief Get identifier as ByteArray with exactly IdLength bytes
     *
     * @return ByteArray The identifier as byte array (always IdLength bytes)
     */
    ByteArray toByteArray() const {
        return ByteArray(m_id.begin(), m_id.end());
    }

    /**
     * @brief Get direct access to the underlying array
     *
     * @return const std::array<uint8_t, ID_LENGTH>& Reference to the internal array
     */
    const std::array<uint8_t, ID_LENGTH>& getArray() const {
        return m_id;
    }

    /**
     * @brief Get pointer to raw data
     *
     * @return const uint8_t* Pointer to the identifier data
     */
    const uint8_t* data() const {
        return m_id.data();
    }

    /**
     * @brief Get the size (always IdLength)
     *
     * @return constexpr size_t Always returns ID_LENGTH (IdLength)
     */
    constexpr size_t size() const {
        return m_id.size();
    }

    /**
     * @brief Check if identifier is all zeros
     *
     * @return bool True if all bytes are zero
     */
    bool isEmpty() const {
        return std::all_of(m_id.begin(), m_id.end(), [](uint8_t byte) { return byte == 0; });
    }

    /**
     * @brief Equality operator
     *
     * @param other The other identifier to compare with
     * @return bool True if both identifiers are identical
     */
    bool operator==(const GenericFixedId& other) const {
        return m_id == other.m_id;
    }

    /**
     * @brief Inequality operator
     *
     * @param other The other identifier to compare with
     * @return bool True if identifiers are different
     */
    bool operator!=(const GenericFixedId& other) const {
        return m_id != other.m_id;
    }

    /**
     * @brief Array subscript operator (const)
     *
     * @param index Index (0 to IdLength-1)
     * @return const uint8_t& Reference to the byte at the given index
     */
    const uint8_t& operator[](size_t index) const {
        return m_id.at(index);
    }

    /**
     * @brief Static instance containing only zeros
     */
    static const GenericFixedId Zero;

private:
    std::array<uint8_t, ID_LENGTH> m_id; ///< The identifier data storage
};

// Definition of the static zero identifier for the template
template<size_t IdLength>
inline const GenericFixedId<IdLength> GenericFixedId<IdLength>::Zero{};

/**
 * @brief Vehicle Identification Number (VIN) - 17 bytes according to ISO 3779
 */
using DoIPVIN = GenericFixedId<17>;

/**
 * @brief Entity Identifier (EID) - 6 bytes for unique entity identification
 */
using DoIPEID = GenericFixedId<6>;

/**
 * @brief Group Identifier (GID) - 6 bytes for group identification
 */
using DoIPGID = GenericFixedId<6>;

} // namespace doip

#endif /* DOIP_IDENTIFIERS_H */


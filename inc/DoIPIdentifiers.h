#ifndef DOIP_IDENTIFIERS_H
#define DOIP_IDENTIFIERS_H

#include "ByteArray.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace doip {

/**
 * @brief Generic fixed-length identifier class template.
 *
 * @tparam IdLength The length of the identifier in bytes.
 * @tparam zeroPadding Whether to use zero-padding (true) or character-padding (false).
 * @tparam padChar The character to use for padding (default is null byte).
 */
template <size_t IdLength, bool zeroPadding = false, char padChar = 0>
class GenericFixedId {
  public:
    /**
     * @brief Length of the identifier in bytes
     */
    static constexpr size_t ID_LENGTH = IdLength;

    /**
     * @brief Default constructor - creates an identifier filled with zeros
     */
    GenericFixedId() : m_id{} { pad(0); }

    /**
     * @brief Construct from string
     *
     * @param id_string The identifier as string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const std::string &id_string) : m_id{} {
        const size_t copy_length = std::min(id_string.length(), ID_LENGTH);
        std::copy_n(id_string.c_str(), copy_length, m_id.data());
        pad(copy_length);
    }

    /**
     * @brief Construct from C-style string
     *
     * @param id_cstr The identifier as C-string. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const char *id_cstr) : m_id{} {
        if (id_cstr != nullptr) {
            const size_t copy_length = std::min(std::strlen(id_cstr), ID_LENGTH);
            std::copy_n(id_cstr, copy_length, m_id.data());
            pad(copy_length);
        } else {
            pad(0);
        }
    }

    /**
     * @brief Construct from byte sequence
     *
     * @param data Pointer to byte data
     * @param length Length of the byte data. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    GenericFixedId(const uint8_t *data, size_t length) : m_id{} {
        if (data != nullptr) {
            const size_t copy_length = std::min(length, ID_LENGTH);
            std::copy_n(data, copy_length, m_id.data());
            pad(copy_length);
        } else {
            pad(0);
        }
    }

    /**
     * @brief Construct from ByteArray
     *
     * @param byte_array The identifier as ByteArray. If shorter than IdLength, padded with zeros. If longer, truncated.
     */
    explicit GenericFixedId(const ByteArray &byte_array) : m_id{} {
        const size_t copy_length = std::min(byte_array.size(), ID_LENGTH);
        std::copy_n(byte_array.data(), copy_length, m_id.data());
        pad(copy_length);
    }

    /**
     * @brief Construct a new Generic Fixed Id object
     *
     * @tparam integral the integral type
     * @param id_value the identifier value as integral type
     */
    template <typename integral,
              typename = std::enable_if_t<std::is_integral_v<integral>>>
    explicit GenericFixedId(integral id_value) : m_id{} {
        //static_assert(ID_LENGTH >= sizeof(integral), "ID_LENGTH is too large for the integral type");
        // big endian!
        constexpr size_t sizeof_integral = sizeof(integral);
        constexpr size_t len = std::min(sizeof_integral, ID_LENGTH);
        // big endian!
        for (size_t i = 0; i < len; ++i) {
            m_id[len - 1 - i] = (id_value >> (i * 8)) & 0xFF;
        }
    }

    /**
     * @brief Copy constructor
     */
    GenericFixedId(const GenericFixedId &other) = default;

    /**
     * @brief Copy assignment operator
     */
    GenericFixedId &operator=(const GenericFixedId &other) = default;

    /**
     * @brief Move constructor
     */
    GenericFixedId(GenericFixedId &&other) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    GenericFixedId &operator=(GenericFixedId &&other) noexcept = default;

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
        return std::string(reinterpret_cast<const char *>(m_id.data()), effective_length);
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
    const std::array<uint8_t, ID_LENGTH> &getArray() const {
        return m_id;
    }

    /**
     * @brief Get pointer to raw data
     *
     * @return const uint8_t* Pointer to the identifier data
     */
    const uint8_t *data() const {
        return m_id.data();
    }

    /**
     * @brief Appends this identifier to the given byte array.
     *
     * @param bytes the byte array to append to
     * @return ByteArray& the modified byte array
     */
    ByteArray &appendTo(ByteArray &bytes) const {
        bytes.insert(bytes.end(), m_id.begin(), m_id.end());
        return bytes;
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
     * @brief Check if identifier is all zeros or all padding characters
     *
     * @return bool True if all bytes are zero or all bytes are the padding character
     */
    bool isEmpty() const {
        if constexpr (zeroPadding) {
            // For zero-padded identifiers, check if all bytes are the padding character
            return std::all_of(m_id.begin(), m_id.end(), [](uint8_t byte) { return byte == static_cast<uint8_t>(padChar); });
        } else {
            // For non-padded identifiers, check if all bytes are zero
            return std::all_of(m_id.begin(), m_id.end(), [](uint8_t byte) { return byte == 0; });
        }
    }

    /**
     * @brief Equality operator
     *
     * @param other The other identifier to compare with
     * @return bool True if both identifiers are identical
     */
    bool operator==(const GenericFixedId &other) const {
        return m_id == other.m_id;
    }

    /**
     * @brief Inequality operator
     *
     * @param other The other identifier to compare with
     * @return bool True if identifiers are different
     */
    bool operator!=(const GenericFixedId &other) const {
        return m_id != other.m_id;
    }

    /**
     * @brief Array subscript operator (const)
     *
     * @param index Index (0 to IdLength-1)
     * @return const uint8_t& Reference to the byte at the given index
     */
    const uint8_t &operator[](size_t index) const {
        return m_id.at(index);
    }

    /**
     * @brief Get the char used to pad shorter identifiers.
     *
     * @return char constexpr the padding character
     */
    char constexpr getPadChar() {
        return padChar;
    }

    /**
     * @brief Get the byte used to pad shorter identifiers.
     *
     * @return uint8_t constexpr the padding byte
     */
    uint8_t constexpr getPadByte() {
        return static_cast<uint8_t>(padChar);
    }

    /**
     * @brief Static instance containing only zeros
     */
    static const GenericFixedId Zero;

  private:
    std::array<uint8_t, ID_LENGTH> m_id; ///< The identifier data storage

    void pad(size_t start_index) {
        if (zeroPadding && start_index < ID_LENGTH) {
            std::fill(m_id.begin() + start_index, m_id.end(), padChar);
        }
    }
};

// Definition of the static zero identifier for the template
template <size_t IdLength, bool zeroPadding, char padChar>
inline const GenericFixedId<IdLength, zeroPadding, padChar> GenericFixedId<IdLength, zeroPadding, padChar>::Zero{};

/**
 * @brief Vehicle Identification Number (VIN) - 17 bytes according to ISO 3779
 * Padded with ASCII '0' characters when shorter than 17 bytes
 */
using DoIPVIN = GenericFixedId<17, true, '0'>;

/**
 * @brief Entity Identifier (EID) - 6 bytes for unique entity identification
 */
using DoIPEID = GenericFixedId<6, false>;

/**
 * @brief Group Identifier (GID) - 6 bytes for group identification
 */
using DoIPGID = GenericFixedId<6, false>;

} // namespace doip

#endif /* DOIP_IDENTIFIERS_H */

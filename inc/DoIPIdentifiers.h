#ifndef DOIP_IDENTIFIERS_H
#define DOIP_IDENTIFIERS_H

#include "ByteArray.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <sstream>
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

    // STL container type aliases
    using value_type = uint8_t;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = uint8_t &;
    using const_reference = const uint8_t &;
    using iterator = typename std::array<uint8_t, IdLength>::iterator;
    using const_iterator = typename std::array<uint8_t, IdLength>::const_iterator;

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
        // static_assert(ID_LENGTH >= sizeof(integral), "ID_LENGTH is too large for the integral type");
        //  big endian!
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
     * @brief Get identifier as hex string, bytes separated by '.'
     *
     * @return std::string Hex representation (e.g. "01.02.0A")
     */
    std::string toHexString() const {
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        for (size_t i = 0; i < ID_LENGTH; ++i) {
            oss << std::setw(2) << static_cast<int>(m_id[i]);
            if (i + 1 < ID_LENGTH) {
                oss << '.';
            }
        }
        return oss.str();
    }

    /**
     * @brief Get identifier as ByteArray with exactly IdLength bytes
     *
     * @return ByteArray The identifier as byte array (always IdLength bytes)
     */
    ByteArrayRef asByteArray() const {
        return {m_id.data(), ID_LENGTH};
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
     * @brief Iterator support - begin
     *
     * @return auto Iterator to the beginning of the identifier data
     */
    auto begin() noexcept { return m_id.begin(); }

    /**
     * @brief Iterator support - begin (const)
     *
     * @return auto Const iterator to the beginning of the identifier data
     */
    auto begin() const noexcept { return m_id.begin(); }

    /**
     * @brief Iterator support - cbegin
     *
     * @return auto Const iterator to the beginning of the identifier data
     */
    auto cbegin() const noexcept { return m_id.cbegin(); }

    /**
     * @brief Iterator support - end
     *
     * @return auto Iterator to the end of the identifier data
     */
    auto end() noexcept { return m_id.end(); }

    /**
     * @brief Iterator support - end (const)
     *
     * @return auto Const iterator to the end of the identifier data
     */
    auto end() const noexcept { return m_id.end(); }

    /**
     * @brief Iterator support - cend
     *
     * @return auto Const iterator to the end of the identifier data
     */
    auto cend() const noexcept { return m_id.cend(); }

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
 *
 * This class enforces VIN validation rules:
 * - Exactly 17 characters
 * - Only uppercase alphanumeric characters (excluding I, O, Q)
 * - Automatically converts lowercase to uppercase
 * - Padded with ASCII '0' characters when shorter than 17 bytes
 */
class DoIpVin {
  private:
    GenericFixedId<17, true, '0'> m_data;

    /**
     * @brief Check if a character is valid for VINs (excluding I, O, Q)
     */
    static bool isValidVinChar(char c) {
        return (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q') ||
               (c >= '0' && c <= '9');
    }

    /**
     * @brief Normalize VIN string: convert to uppercase
     *
     * @param vin Input VIN string
     * @param validate If true, throws on invalid characters
     * @return Normalized VIN string (uppercase)
     * @throws std::invalid_argument if validate=true and VIN contains invalid characters
     */
    static std::string normalizeVin(std::string vin, bool validate = true) {
        // Convert to uppercase
        std::transform(vin.begin(), vin.end(), vin.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        if (validate) {
            // Validate characters
            if (!std::all_of(vin.begin(), vin.end(),
                             [](char c) { return isValidVinChar(c) || c == '0'; })) {
                return "00000000000000000";
            }
        }
        return vin;
    }

      public:
        /// Length of VIN in bytes (ISO 3779)
        static constexpr size_t VIN_LENGTH = 17;

        /// Static zero-initialized VIN instance
        static const DoIpVin Zero;

        /**
         * @brief Default constructor - creates VIN filled with '0' padding
         */
        DoIpVin() : m_data() {}

        /**
         * @brief Construct from string with automatic uppercase conversion
         *
         * @param vin VIN as string. Converted to uppercase and padded/truncated to 17 bytes.
         *            Does NOT validate character set to maintain backward compatibility.
         *            Use isValid() to check validity after construction.
         */
        explicit DoIpVin(const std::string &vin)
            : m_data(normalizeVin(vin, false)) {}

        /**
         * @brief Construct from C-style string with automatic uppercase conversion
         *
         * @param vin VIN as C-string. Converted to uppercase and padded/truncated to 17 bytes.
         *            Does NOT validate character set to maintain backward compatibility.
         *            Use isValid() to check validity after construction.
         */
        explicit DoIpVin(const char *vin)
            : m_data(vin ? normalizeVin(std::string(vin), false) : std::string()) {} /**
                                                                                      * @brief Construct from byte sequence (no validation, assumes pre-validated data)
                                                                                      *
                                                                                      * @param data Pointer to byte data
                                                                                      * @param length Length of byte data
                                                                                      */
        DoIpVin(const uint8_t *data, size_t length)
            : m_data(data, length) {}

        /**
         * @brief Construct from ByteArray (no validation, assumes pre-validated data)
         *
         * @param byte_array VIN as ByteArray
         */
        explicit DoIpVin(const ByteArray &byte_array)
            : m_data(byte_array) {}

        /**
         * @brief Construct from integral value (for numeric VINs)
         *
         * @tparam Integral Integral type
         * @param id_value Numeric VIN value
         */
        template <typename Integral,
                  typename = std::enable_if_t<std::is_integral_v<Integral>>>
        explicit DoIpVin(Integral id_value)
            : m_data(id_value) {}

        // Default copy/move semantics
        DoIpVin(const DoIpVin &) = default;
        DoIpVin &operator=(const DoIpVin &) = default;
        DoIpVin(DoIpVin &&) noexcept = default;
        DoIpVin &operator=(DoIpVin &&) noexcept = default;
        ~DoIpVin() = default;

        // Delegate to underlying GenericFixedId
        std::string toString() const {
            return m_data.toString();
        }
        std::string toHexString() const {
            return m_data.toHexString();
        }
        ByteArrayRef asByteArray() const {
            return m_data.asByteArray();
        }
        const std::array<uint8_t, 17> &getArray() const {
            return m_data.getArray();
        }
        const uint8_t *data() const {
            return m_data.data();
        }
        ByteArray &appendTo(ByteArray & bytes) const {
            return m_data.appendTo(bytes);
        }
        constexpr size_t size() const {
            return VIN_LENGTH;
        }
        bool isEmpty() const {
            return m_data.isEmpty();
        }

        bool operator==(const DoIpVin &other) const {
            return m_data == other.m_data;
        }
        bool operator!=(const DoIpVin &other) const {
            return m_data != other.m_data;
        }
        const uint8_t &operator[](size_t index) const {
            return m_data[index];
        }

        constexpr char getPadChar() const {
            return '0';
        }
        constexpr uint8_t getPadByte() const {
            return static_cast<uint8_t>('0');
        }

        // Iterator support
        auto begin() noexcept {
            return m_data.begin();
        }
        auto begin() const noexcept {
            return m_data.begin();
        }
        auto cbegin() const noexcept {
            return m_data.cbegin();
        }
        auto end() noexcept {
            return m_data.end();
        }
        auto end() const noexcept {
            return m_data.end();
        }
        auto cend() const noexcept {
            return m_data.cend();
        }

        /**
         * @brief Validate VIN according to ISO 3779 character set
         *
         * @return true if all characters are valid (A-Z except I,O,Q and 0-9)
         */
        bool isValid() const {
            return std::all_of(m_data.begin(), m_data.end(),
                               [](uint8_t byte) {
                                   char c = static_cast<char>(byte);
                                   return isValidVinChar(c) || c == '0';
                               });
        }
    };

    // Definition of static Zero instance
    inline const DoIpVin DoIpVin::Zero{};

    /**
     * @brief Validate a VIN according to allowed characters (excluding I, O, Q)
     *
     * @param vin The DoIpVin to validate
     * @return true if valid, false otherwise
     */
    inline bool isValidVin(const DoIpVin &vin) {
        return vin.isValid();
    }

    /**
     * @brief Entity Identifier (EID) - 6 bytes for unique entity identification
     */
    using DoIpEid = GenericFixedId<6, false>;

    /**
     * @brief Group Identifier (GID) - 6 bytes for group identification
     */
    using DoIpGid = GenericFixedId<6, false>;

    /**
     * @brief Stream output operator for DoIpVin, DoIpEid, and DoIpGid
     *
     * @param os the operation stream
     * @param vin the DoIpVin to output
     * @return std::ostream& the operation stream
     */
    inline std::ostream &operator<<(std::ostream &os, const DoIpVin &vin) {
        os << vin.toString();
        return os;
    }

    /**
     * @brief Stream output operator for DoIpEid/DoIpGid
     *
     * @param os the operation stream
     * @param eid the DoIpEid/DoIpGid to output
     * @return std::ostream&  @ref {type}  ["{type}"]   //  @return Returns @c true in the case of success, @c false otherwise.
     */
    inline std::ostream &operator<<(std::ostream &os, const DoIpEid &eid) {
        os << eid.toHexString();
        return os;
    }

} // namespace doip

#endif /* DOIP_IDENTIFIERS_H */

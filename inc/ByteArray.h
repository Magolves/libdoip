#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <stdint.h>
#include <vector>
#include <ostream>
#include <iomanip>

namespace doip {

using ByteArray = std::vector<uint8_t>;

/**
 * @brief Reference to raw array of bytes.
 */
using ByteArrayRef = std::pair<const uint8_t*, size_t>;

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
inline std::ostream& operator<<(std::ostream& os, const ByteArray& arr) {
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

}




#endif  /* BYTEARRAY_H */

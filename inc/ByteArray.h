#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <stdint.h>
#include <vector>

namespace doip {

using ByteArray = std::vector<uint8_t>;

/**
 * @brief Reference to raw array of bytes.
 */
using ByteArrayRef = std::pair<const uint8_t*, size_t>;

}


#endif  /* BYTEARRAY_H */

#ifndef DOIPSYNCSTATUS_H
#define DOIPSYNCSTATUS_H

#include <stdint.h>

namespace doip {
    // Table 7 - VIN/GID Sync Status values
    enum class DoIPSyncStatus : uint8_t {
        GidVinSynchronized = 0x00,
        // 0x01 to 0x0F: reserved
        GidVinNotSynchronized = 0x10,
        // 0x11 to 0xFF: reserved
    };
} // namespace doip

#endif /* DOIPSYNCSTATUS_H */

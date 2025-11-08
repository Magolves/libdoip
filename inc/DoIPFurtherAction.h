#ifndef DOIPFURTHERACTION_H
#define DOIPFURTHERACTION_H

#include <stdint.h>

namespace doip {
    // Table 6 - Further Action Required values
    enum class DoIPFurtherAction : uint8_t {
        NoFurtherAction = 0x00,
        // 0x01 to 0x0F: reserved
        RoutingActivationForCentralSecurity = 0x10,
        // 0x11 to 0xFE: reserved for VM manufacturer specific use
    };
} // namespace doip

#endif /* DOIPFURTHERACTION_H */

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

    inline std::ostream &operator<<(std::ostream &os, const DoIPFurtherAction far) {
    switch (far) {
        case DoIPFurtherAction::NoFurtherAction:
            os << "None";
            break;
        case DoIPFurtherAction::RoutingActivationForCentralSecurity:
            os << "Routing Activation for Central Security Required";
            break;
        default:
            os << "Reserved Further Action Code: 0x" << std::hex << static_cast<uint8_t>(far) << std::dec;
            break;
    }

    return os;
    }

} // namespace doip

#endif /* DOIPFURTHERACTION_H */

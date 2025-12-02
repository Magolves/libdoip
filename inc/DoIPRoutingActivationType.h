#ifndef DOIPACTIVATIONTYPE_H
#define DOIPACTIVATIONTYPE_H

#include <stdint.h>

// Table 54
enum class DoIPRoutingActivationType : uint8_t {
    Default = 0,
    DiagnosticCommRequired  = 1,
    // 0x02 - 0xDF: reserved
    CentralSecurity = 0xE0,

};

constexpr std::optional<DoIPRoutingActivationType> toRoutingActivationType(uint8_t value) noexcept {
    switch(value) {
        case 0: return DoIPRoutingActivationType::Default;
        case 0xE0: return DoIPRoutingActivationType::CentralSecurity;
        default: break;
    }

    return std::nullopt;
}

#endif /* DOIPACTIVATIONTYPE_H */

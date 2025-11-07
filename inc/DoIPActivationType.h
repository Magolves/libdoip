#ifndef DOIPACTIVATIONTYPE_H
#define DOIPACTIVATIONTYPE_H

#include <stdint.h>

// Table 54
enum class DoIPActivationType : uint8_t {
    Default = 0,
    DiagnosticCommRequired  = 1,
    // 0x02 - 0xDF: reserved
    CentralSecurity = 0xE0,
    
};

#endif /* DOIPACTIVATIONTYPE_H */

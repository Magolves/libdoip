#ifndef DOIPNEGATIVEDIAGNOSTICACK_H
#define DOIPNEGATIVEDIAGNOSTICACK_H

#include <stdint.h>

// Table 26
enum class DoIPNegativeDiagnosticAck : uint8_t {
    // 0 and 1: reserved
    InvalidSourceAddress = 2,
    UnknownTargetAddress = 3,
    DiagnosticMessageTooLarge = 4,
    OutOfMemory = 5,
    TargetUnreachable = 6,
    UnknownNetwork = 7,
    TransportProtocolError = 8, // also use if other error codes do not apply
    TargetBusy = 9,
};

#endif  /* DOIPNEGATIVEDIAGNOSTICACK_H */

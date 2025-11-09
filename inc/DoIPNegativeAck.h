#ifndef DOIPNEGATIVEACK_H
#define DOIPNEGATIVEACK_H

#include <stdint.h>

// Table 19: Negative Acknowledgement Codes
enum class DoIPNegativeAck : uint8_t {
    IncorrectPatternFormat = 0, // Invalid header pattern
    UnknownPayloadType =1,
    MessageTooLarge = 2,
    OutOfMemory = 3,
    InvalidPayloadLength = 4
};


#endif  /* DOIPNEGATIVEACK_H */

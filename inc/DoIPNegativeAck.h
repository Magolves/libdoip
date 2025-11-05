#ifndef DOIPNEGATIVEACK_H
#define DOIPNEGATIVEACK_H

#include <stdint.h>

enum class DoIPNegativeAck : uint8_t {
    IncorrectPatternFormat = 0,
    UnknownPayloadType =1,
    MessageTooLarge = 2,
    OutOfMemory = 3,
    InvalidPayloadLength = 4
};


#endif  /* DOIPNEGATIVEACK_H */

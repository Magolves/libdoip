#ifndef DOIPGENERICHEADERHANDLER_H
#define DOIPGENERICHEADERHANDLER_H

#include <stdint.h>

#include "DoIPAddress.h"

const int _GenericHeaderLength = 8;
const int _NACKLength = 1;

namespace doip {

const uint8_t _IncorrectPatternFormatCode = 0x00;
const uint8_t _UnknownPayloadTypeCode = 0x01;
const uint8_t _InvalidPayloadLengthCode = 0x04;


enum PayloadType : uint8_t {
    NEGATIVEACK,
    ROUTINGACTIVATIONREQUEST,
    ROUTINGACTIVATIONRESPONSE,
    VEHICLEIDENTREQUEST,
    VEHICLEIDENTRESPONSE,
    DIAGNOSTICMESSAGE,
    DIAGNOSTICPOSITIVEACK,
    DIAGNOSTICNEGATIVEACK,
    ALIVECHECKRESPONSE,
};

struct GenericHeaderAction {
    PayloadType type;
    uint8_t value;
    unsigned long payloadLength;
};

GenericHeaderAction parseGenericHeader(uint8_t *data, size_t dataLenght);
uint8_t *createGenericHeader(PayloadType type, size_t length);

} // namespace doip

#endif /* DOIPGENERICHEADERHANDLER_H */

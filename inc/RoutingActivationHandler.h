#ifndef ROUTINGACTIVATIONHANDLER_H
#define ROUTINGACTIVATIONHANDLER_H

#include <arpa/inet.h>
#include <netinet/in.h>

#include "DoIPAddress.h"

namespace doip {

const int _ActivationResponseLength = 9;

const uint8_t _UnknownSourceAddressCode = 0x00;
const uint8_t _UnsupportedRoutingTypeCode = 0x06;
const uint8_t _SuccessfullyRoutedCode = 0x10;

uint8_t parseRoutingActivation(uint8_t *data);
uint8_t *createRoutingActivationResponse(const DoIPAddress &sourceAddress,
                                         const DoIPAddress &clientAddress,
                                         uint8_t responseCode);

} // namespace doip

#endif /* ROUTINGACTIVATIONHANDLER_H */

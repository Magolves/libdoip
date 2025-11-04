#ifndef VEHICLEIDENTIFICATIONHANDLER_H
#define VEHICLEIDENTIFICATIONHANDLER_H

#include "DoIPGenericHeaderHandler.h"
#include <string>

uint8_t *createVehicleIdentificationResponse(std::string VIN, const DoIPAddress &LogicalAdress, uint8_t *EID, uint8_t *GID, uint8_t FurtherActionReq);

const int _VIResponseLength = 32;

#endif /* VEHICLEIDENTIFICATIONHANDLER_H */

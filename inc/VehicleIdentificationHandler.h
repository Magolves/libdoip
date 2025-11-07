#ifndef VEHICLEIDENTIFICATIONHANDLER_H
#define VEHICLEIDENTIFICATIONHANDLER_H

#include <string>
#include "DoIPAddress.h"

namespace doip {

uint8_t *createVehicleIdentificationResponse(std::string VIN, const DoIPAddress &LogicalAdress, uint8_t *EID, uint8_t *GID, uint8_t FurtherActionReq);

const int _VIResponseLength = 32;

} // namespace doip

#endif /* VEHICLEIDENTIFICATIONHANDLER_H */

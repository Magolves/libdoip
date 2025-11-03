#include "VehicleIdentificationHandler.h"
#include <iostream>

uint8_t* createVehicleIdentificationResponse(std::string VIN,unsigned short LogicalAddress,
                                                    uint8_t* EID, uint8_t* GID,
                                                    uint8_t FurtherActionReq) //also used für the Vehicle Announcement
{
    uint8_t* message = createGenericHeader(PayloadType::VEHICLEIDENTRESPONSE, _VIResponseLength);

    //VIN Number
    unsigned int j = 0;
    for(int i = 8; i <= 24; i++)
    {
        if (j < VIN.length()){
            message[i] = static_cast<uint8_t>(VIN[j]);
            j++;
	} else {
	    //Pad with ASCII '0' if VIN is shorter than 17 bytes
	    message[i] = static_cast<uint8_t>('0');
	}
    }

    //Logical Adress
    message[25] = (LogicalAddress >> 8) & 0xFF;
    message[26] = LogicalAddress & 0xFF;

    //EID
    j = 0;
    for(int i = 27; i <= 32; i++)
    {
        message[i] = EID[j];
        j++;
    }

    //GID
    j = 0;
    for(int i = 33; i <= 38; i++)
    {
        message[i] = GID[j];
        j++;
    }

    message[39] = FurtherActionReq;

    return message;
}


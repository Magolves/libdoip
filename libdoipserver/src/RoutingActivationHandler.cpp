#include "RoutingActivationHandler.h"

/**
 * Checks if the Routing Activation Request is valid
 * @param data  contains the request
 * @return      routing activation response code
 */
uint8_t parseRoutingActivation(uint8_t *data) {

    // Check if source address is known
    DoIPAddress address(data, 8);
    if (!address.isValidSourceAddress()) {
        // send routing activation negative response code --> close socket
        return _UnknownSourceAddressCode;
    }

    // Check if routing activation type is supported
    switch (data[2]) {
    case 0x00: {
        // Activation Type default
        break;
    }
    case 0x01: {
        // Activation Type WWH-OBD
        break;
    }
    default: {
        // unknown activation type
        // send routing activation negative response code --> close socket
        return _UnsupportedRoutingTypeCode;
    }
    }

    // if not exited before, send routing activation positive response
    return _SuccessfullyRoutedCode;
}

/**
 * Create the complete routing activation response, which also contains the
 * generic header
 * @param clientAddress     address of the test equipment
 * @param responseCode      routing activation response code
 * @return                  complete routing activation response
 */
uint8_t *createRoutingActivationResponse(const DoIPAddress &sourceAddress, const DoIPAddress &clientAddress,
                                         uint8_t responseCode) {

    uint8_t *message = createGenericHeader(PayloadType::ROUTINGACTIVATIONRESPONSE,
                                           _ActivationResponseLength);

    // Logical address of external test equipment
    message[8] = clientAddress.hsb();
    message[9] = clientAddress.lsb();

    // logical address of DoIP entity
    message[10] = sourceAddress.hsb();
    message[11] = sourceAddress.lsb();

    // routing activation response code
    message[12] = responseCode;

    // reserved for future standardization use
    message[13] = 0x00;
    message[14] = 0x00;
    message[15] = 0x00;
    message[16] = 0x00;

    return message;
}

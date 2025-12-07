#ifndef DOIPROUTINGACTIVATIONRESULT_H
#define DOIPROUTINGACTIVATIONRESULT_H

namespace doip {

// Table 56 - Routing activation response codes
enum class DoIPRoutingActivationResult : uint8_t{
    UnknownSourceAddress = 0x00,           // close socket
    NoMoreRoutingSlotsAvailable = 0x01,    // no more routing slots available, close socket
    InvalidAddressOrRoutingType = 0x02,    // Source address or routing type differ from already registered one
    SourceAddressAlreadyRegistered = 0x03, // source address already registered, close socket
    Unauthorized = 0x04,                   // unauthorized source address, do not activate routing but keep socket open
    MissingConfirmation = 0x05,            // missing confirmation, close socket
    InvalidRoutingType = 0x06,             // invalid routing type, close socket
    SecuredConnectionRequired = 0x07,      // secured connection (TLS) required, close socket
    VehicleNotReadyForRouting = 0x08,      // vehicle not ready for routing (e. g. pending update), do not activate routing but keep socket open
    // 0x09: reserved
    RouteActivated = 0x10,                    // routing activated successfully
    RouteActivatedConfirmationRequired = 0x11 // routing activated successfully but requires confirmation (optional)
};

/**
 * @brief Check if the socket should be closed based on the routing activation result.
 *
 * @param result the routing activation result code
 * @return true if the socket should be closed, false otherwise
 */
inline bool closeSocketOnRoutingActivationResult(DoIPRoutingActivationResult result) {
    switch (result) {
    case DoIPRoutingActivationResult::UnknownSourceAddress:
    case DoIPRoutingActivationResult::NoMoreRoutingSlotsAvailable:
    case DoIPRoutingActivationResult::SourceAddressAlreadyRegistered:
    case DoIPRoutingActivationResult::MissingConfirmation:
    case DoIPRoutingActivationResult::InvalidRoutingType:
    case DoIPRoutingActivationResult::SecuredConnectionRequired:
        return true;
    case DoIPRoutingActivationResult::InvalidAddressOrRoutingType:
    case DoIPRoutingActivationResult::Unauthorized:
    case DoIPRoutingActivationResult::VehicleNotReadyForRouting:
    case DoIPRoutingActivationResult::RouteActivated:
    case DoIPRoutingActivationResult::RouteActivatedConfirmationRequired:
        return false;
    default:
        return true; // unknown code, be safe and close socket
    }
}

inline std::ostream& operator<<(std::ostream& os, DoIPRoutingActivationResult result) {
    switch (result) {
        case DoIPRoutingActivationResult::UnknownSourceAddress:
            return os << "UnknownSourceAddress";
        case DoIPRoutingActivationResult::NoMoreRoutingSlotsAvailable:
            return os << "NoMoreRoutingSlotsAvailable";
        case DoIPRoutingActivationResult::InvalidAddressOrRoutingType:
            return os << "InvalidAddressOrRoutingType";
        case DoIPRoutingActivationResult::SourceAddressAlreadyRegistered:
            return os << "SourceAddressAlreadyRegistered";
        case DoIPRoutingActivationResult::Unauthorized:
            return os << "Unauthorized";
        case DoIPRoutingActivationResult::MissingConfirmation:
            return os << "MissingConfirmation";
        case DoIPRoutingActivationResult::InvalidRoutingType:
            return os << "InvalidRoutingType";
        case DoIPRoutingActivationResult::SecuredConnectionRequired:
            return os << "SecuredConnectionRequired";
        case DoIPRoutingActivationResult::VehicleNotReadyForRouting:
            return os << "VehicleNotReadyForRouting";
        case DoIPRoutingActivationResult::RouteActivated:
            return os << "RouteActivated";
        case DoIPRoutingActivationResult::RouteActivatedConfirmationRequired:
            return os << "RouteActivatedConfirmationRequired";
        default:
            return os << "Unknown(" << static_cast<int>(result) << ")";
    }
}

} // namespace doip

#endif /* DOIPROUTINGACTIVATIONRESULT_H */

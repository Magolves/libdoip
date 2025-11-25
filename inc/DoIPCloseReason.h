#ifndef DOIPCLOSEREASON_H
#define DOIPCLOSEREASON_H

#include <cstdint>
#include <iostream>

namespace doip {

/**
 * @brief Reason for connection closure
 */
enum class DoIPCloseReason : uint8_t {
    None,
    InitialInactivityTimeout,
    GeneralInactivityTimeout,
    AliveCheckTimeout,
    SocketError,
    InvalidMessage,
    ApplicationRequest,
    RoutingActivationDenied
};

inline std::ostream &operator<<(std::ostream &os, DoIPCloseReason event) {
    switch (event) {
    case DoIPCloseReason::None:
        return os << "None";

    case DoIPCloseReason::InitialInactivityTimeout:
        return os << "Initial Inactivity Timeout";
    case DoIPCloseReason::GeneralInactivityTimeout:
        return os << "General Inactivity Timeout";
    case DoIPCloseReason::AliveCheckTimeout:
        return os << "AliveCheck Timeout";
    case DoIPCloseReason::SocketError:
        return os << "Socket Error";
    case DoIPCloseReason::InvalidMessage:
        return os << "Invalid Message";
    case DoIPCloseReason::ApplicationRequest:
        return os << "Application Request";
    case DoIPCloseReason::RoutingActivationDenied:
        return os << "Routing Activation Denied";
    default:
        return os << "Unknown(" << static_cast<int>(event) << ")";
    }
}

} // namespace doip

#endif /* DOIPCLOSEREASON_H */

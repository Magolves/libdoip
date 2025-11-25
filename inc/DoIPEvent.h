#ifndef DOIPEVENT_H
#define DOIPEVENT_H

#include <iostream>

namespace doip {

// DoIP Events
enum class DoIPEvent {
    // Message events
    RoutingActivationReceived,
    AliveCheckResponseReceived,
    DiagnosticMessageReceived,
    CloseRequestReceived,

    // Timer events
    Initial_inactivity_timeout,
    GeneralInactivityTimeout,
    AliveCheckTimeout,

    // Error events
    InvalidMessage,
    SocketError
};

// Stream operator for DoIPEvent
inline std::ostream &operator<<(std::ostream &os, DoIPEvent event) {
    switch (event) {
    case DoIPEvent::RoutingActivationReceived:
        return os << "RoutingActivationReceived";
    case DoIPEvent::AliveCheckResponseReceived:
        return os << "AliveCheckResponseReceived";
    case DoIPEvent::DiagnosticMessageReceived:
        return os << "DiagnosticMessageReceived";
    case DoIPEvent::CloseRequestReceived:
        return os << "CloseRequestReceived";
    case DoIPEvent::Initial_inactivity_timeout:
        return os << "Initial_inactivity_timeout";
    case DoIPEvent::GeneralInactivityTimeout:
        return os << "GeneralInactivityTimeout";
    case DoIPEvent::AliveCheckTimeout:
        return os << "AliveCheckTimeout";
    case DoIPEvent::InvalidMessage:
        return os << "InvalidMessage";
    case DoIPEvent::SocketError:
        return os << "SocketError";
    default:
        return os << "Unknown(" << static_cast<int>(event) << ")";
    }
}
} // namespace doip

#endif /* DOIPEVENT_H */

#ifndef DOIPEVENT_H
#define DOIPEVENT_H

#include <iostream>

namespace doip {

// DoIP Events
enum class DoIPServerEvent {
    // Message events
    RoutingActivationReceived,
    AliveCheckResponseReceived,
    DiagnosticMessageReceived,
    DiagnosticMessageReceivedDownstream,
    CloseRequestReceived,

    // Timer events
    Initial_inactivity_timeout,
    GeneralInactivityTimeout,
    AliveCheckTimeout,
    DownstreamTimeout,

    // Error events
    InvalidMessage,
    SocketError
};

// Stream operator for DoIPServerEvent
inline std::ostream &operator<<(std::ostream &os, DoIPServerEvent event) {
    switch (event) {
    case DoIPServerEvent::RoutingActivationReceived:
        return os << "RoutingActivationReceived";
    case DoIPServerEvent::AliveCheckResponseReceived:
        return os << "AliveCheckResponseReceived";
    case DoIPServerEvent::DiagnosticMessageReceived:
        return os << "DiagnosticMessageReceived";
    case DoIPServerEvent::DiagnosticMessageReceivedDownstream:
        return os << "DiagnosticMessageReceivedDownstream";
    case DoIPServerEvent::CloseRequestReceived:
        return os << "CloseRequestReceived";
    case DoIPServerEvent::Initial_inactivity_timeout:
        return os << "Initial_inactivity_timeout";
    case DoIPServerEvent::GeneralInactivityTimeout:
        return os << "GeneralInactivityTimeout";
    case DoIPServerEvent::AliveCheckTimeout:
        return os << "AliveCheckTimeout";
    case DoIPServerEvent::InvalidMessage:
        return os << "InvalidMessage";
    case DoIPServerEvent::SocketError:
        return os << "SocketError";
    case DoIPServerEvent::DownstreamTimeout:
        return os << "DownstreamTimeout";
    default:
        return os << "Unknown(" << static_cast<int>(event) << ")";
    }
}
} // namespace doip

#endif /* DOIPEVENT_H */

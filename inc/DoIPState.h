#ifndef DOIPSTATE_H
#define DOIPSTATE_H

#include <iostream>

namespace doip {
// DoIP Protocol States
// Figure 26
enum class DoIPState {
    SocketInitialized,      // Initial state after socket creation
    WaitRoutingActivation,  // Waiting for routing activation request
    RoutingActivated,       // Routing is active, ready for diagnostics
    WaitAliveCheckResponse, // D& config  Waiting for alive check response
    Finalize,               // Cleanup state
    Closed                  // Connection closed
};

// Stream operator for DoIPState
inline std::ostream &operator<<(std::ostream &os, DoIPState state) {
    switch (state) {
    case DoIPState::SocketInitialized:
        return os << "SocketInitialized";
    case DoIPState::WaitRoutingActivation:
        return os << "WaitRoutingActivation";
    case DoIPState::RoutingActivated:
        return os << "RoutingActivated";
    case DoIPState::WaitAliveCheckResponse:
        return os << "WaitAliveCheckResponse";
    case DoIPState::Finalize:
        return os << "Finalize";
    case DoIPState::Closed:
        return os << "Closed";
    default:
        return os << "Unknown(" << static_cast<int>(state) << ")";
    }
}

} // namespace doip

#endif /* DOIPSTATE_H */

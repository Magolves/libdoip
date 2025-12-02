#ifndef DOIPSTATE_H
#define DOIPSTATE_H

#include <iostream>

namespace doip {
// DoIP Protocol States
// Figure 26
enum class DoIPServerState {
    SocketInitialized,      // Initial state after socket creation
    WaitRoutingActivation,  // Waiting for routing activation request
    RoutingActivated,       // Routing is active, ready for diagnostics
    WaitAliveCheckResponse, // D& config  Waiting for alive check response
    WaitDownstreamResponse, // Waiting for response from downstream device
    Finalize,               // Cleanup state
    Closed                  // Connection closed
};

// Stream operator for DoIPServerState
inline std::ostream &operator<<(std::ostream &os, DoIPServerState state) {
    switch (state) {
    case DoIPServerState::SocketInitialized:
        return os << "SocketInitialized";
    case DoIPServerState::WaitRoutingActivation:
        return os << "WaitRoutingActivation";
    case DoIPServerState::RoutingActivated:
        return os << "RoutingActivated";
    case DoIPServerState::WaitAliveCheckResponse:
        return os << "WaitAliveCheckResponse";
    case DoIPServerState::WaitDownstreamResponse:
        return os << "WaitDownstreamResponse";
    case DoIPServerState::Finalize:
        return os << "Finalize";
    case DoIPServerState::Closed:
        return os << "Closed";
    default:
        return os << "Unknown(" << static_cast<int>(state) << ")";
    }
}

} // namespace doip

#endif /* DOIPSTATE_H */

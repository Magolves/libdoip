#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "ByteArray.h"

namespace doip {

/**
 * Result of a downstream request.
 */
enum class DownstreamStatus {
    Handled, ///< Successfully handled
    Pending, ///< Still in progress
    Timeout, ///< No response in time
    Error    ///< Subnet or provider error
};

/**
 * Structured downstream response.
 */
struct DownstreamResponse {
    ByteArray payload;                                 ///< Raw UDS/CAN/etc. response
    DownstreamStatus status = DownstreamStatus::Error; ///< Result status
    std::chrono::milliseconds latency{0};              ///< Time taken for the response
};

/// Callback used when the provider completes a request.
using DownstreamCallback = std::function<void(const DownstreamResponse &)>;

/**
 * Interface for downstream communication providers.
 * Examples: SocketCANProvider, UdsMockProvider, ...
 */
class IDownstreamProvider {
  public:
    virtual ~IDownstreamProvider() = default;

    /**
     * Send a request to the downstream subnet and return asynchronously.
     * The provider must call the callback exactly once.
     * @param request The raw request payload to send downstream
     * @param cb The callback to invoke when the response is received
     */

    virtual void sendRequest(const ByteArray request, DownstreamCallback cb) = 0;

    /**
     * @brief Start the provider (e.g., open sockets, threads, etc.)
     */
    virtual void start() {}

    /**
     * @brief Stop the provider and clean up resources.
     */
    virtual void stop() {}
};

} // namespace doip
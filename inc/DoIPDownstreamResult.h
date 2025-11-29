#ifndef DOIPDOWNSTREAMRESULT_H
#define DOIPDOWNSTREAMRESULT_H

namespace doip {

/**
 * @brief Result of a downstream request initiation
 */
enum class DoIPDownstreamResult {
    Pending,        ///< Request was sent, waiting for async response
    Handled,        ///< Request was handled synchronously (no downstream needed)
    Error           ///< Failed to initiate downstream request
};

} // namespace doip

#endif /* DOIPDOWNSTREAMRESULT_H */

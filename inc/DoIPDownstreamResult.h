#ifndef DOIPDOWNSTREAMRESULT_H
#define DOIPDOWNSTREAMRESULT_H

#include "AnsiColors.h"

namespace doip {

/**
 * @brief Result of a downstream request initiation
 */
enum class DoIPDownstreamResult {
    Pending,        ///< Request was sent, waiting for async response
    Handled,        ///< Request was handled synchronously (no downstream needed)
    Error           ///< Failed to initiate downstream request
};

inline std::ostream &operator<<(std::ostream &os, DoIPDownstreamResult result) {
    switch(result) {
        case DoIPDownstreamResult::Pending:
            os << ansi::yellow << "Pending" << ansi::reset;
            break;
        case DoIPDownstreamResult::Handled:
            os << ansi::green << "Handled" << ansi::reset;
            break;
        case DoIPDownstreamResult::Error:
            os << ansi::red << "Error" << ansi::reset;
            break;
        default:
            os << ansi::red << "Unknown value for DoIPDownstreamResult: " << static_cast<int>(result) << ansi::reset;
            break;
    }
    return os;
}

} // namespace doip

#endif /* DOIPDOWNSTREAMRESULT_H */

#ifndef DIAGNOSTICMESSAGEHANDLER_H
#define DIAGNOSTICMESSAGEHANDLER_H

#include "DoIPGenericHeaderHandler.h"
#include "ByteArray.h"
#include "DoIPNegativeDiagnosticAck.h"
#include <functional>
#include <optional>

namespace doip {

using DiagnosticCallback = std::function<void(const DoIPAddress&, const uint8_t *, size_t)>;
using DiagnosticMessageNotification = std::function<bool(const DoIPAddress&)>;

const size_t _DiagnosticPositiveACKLength = 5;
const size_t _DiagnosticMessageMinimumLength = 4;

const uint8_t _ValidDiagnosticMessageCode = 0x00;
const uint8_t _InvalidSourceAddressCode = 0x02;
const uint8_t _UnknownTargetAddressCode = 0x03;

/**
 * Checks if a received Diagnostic Message is valid
 * @param callback                   callback which will be called with the user data
 * @param sourceAddress		currently registered source address on the socket
 * @param data			message which was received
 * @param length     length of the diagnostic message
 * @return std::optional<DoIPNegativeDiagnosticAck>  std::nullopt if message is valid, NACK code otherwise
 */
DoIPDiagnosticAck handleDiagnosticMessage(DiagnosticCallback callback, const DoIPAddress& sourceAddress, const uint8_t *data, size_t length);

} // namespace doip

#endif /* DIAGNOSTICMESSAGEHANDLER_H */

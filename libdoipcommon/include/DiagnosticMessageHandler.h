#ifndef DIAGNOSTICMESSAGEHANDLER_H
#define DIAGNOSTICMESSAGEHANDLER_H

#include "DoIPGenericHeaderHandler.h"
#include <functional>

using DiagnosticCallback = std::function<void(const DoIPAddress&, uint8_t *, size_t)>;
using DiagnosticMessageNotification = std::function<bool(const DoIPAddress&)>;

const size_t _DiagnosticPositiveACKLength = 5;
const size_t _DiagnosticMessageMinimumLength = 4;

const uint8_t _ValidDiagnosticMessageCode = 0x00;
const uint8_t _InvalidSourceAddressCode = 0x02;
const uint8_t _UnknownTargetAddressCode = 0x03;

uint8_t parseDiagnosticMessage(DiagnosticCallback callback, const DoIPAddress& sourceAddress, const uint8_t *data, size_t diagMessageLength);
uint8_t *createDiagnosticACK(bool ackType, const DoIPAddress& sourceAddress, const DoIPAddress&  targetAddress, uint8_t responseCode);
uint8_t *createDiagnosticMessage(const DoIPAddress& sourceAddress, const DoIPAddress&  targetAddress, uint8_t *userData, size_t userDataLength);

#endif /* DIAGNOSTICMESSAGEHANDLER_H */

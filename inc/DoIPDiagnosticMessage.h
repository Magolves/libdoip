#ifndef DOIPDIAGNOSTICMESSAGE_H
#define DOIPDIAGNOSTICMESSAGE_H

#include "DoIPMessage.h"
namespace doip {


struct DoIPDiagnosticMessage : DoIPMessage {
    DoIPDiagnosticMessage(const DoIPAddress& sourceAddress, const DoIPAddress& targetAddress, const ByteArray& payload)
        : DoIPMessage(DoIPPayloadType::DiagnosticMessage, buildPayload(sourceAddress, targetAddress, payload)) {}



private:
    DoIPAddress m_source_address;
    DoIPAddress m_target_address;

    static ByteArray buildPayload(const DoIPAddress& sourceAddress, const DoIPAddress& targetAddress, const ByteArray& payload)
    {
        ByteArray fullPayload;
        fullPayload.reserve(sourceAddress.size() + payload.size());
        sourceAddress.appendTo(fullPayload);
        targetAddress.appendTo(fullPayload);
        fullPayload.insert(fullPayload.end(), payload.begin(), payload.end());
        return fullPayload;
    }


};

} // namespace doip
#endif /* DOIPDIAGNOSTICMESSAGE_H */

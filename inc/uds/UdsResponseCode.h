#ifndef UDSRESPONSE_H
#define UDSRESPONSE_H

#include <stdint.h>

#include "AnsiColors.h"
#include <iostream>
#include <iomanip>

namespace doip::uds {
    enum class UdsResponseCode : uint8_t {
        OK = 0, // positive response
        // Negative Response Codes (NRCs) as defined in ISO14229-1:2020 Table A.1 - Negative Response
        // Code (NRC) definition and values
        PositiveResponse = 0x0,
        // 0x01 to 0x0F are reserved by ISO14229-1:2020
        GeneralReject = 0x10,
        ServiceNotSupported = 0x11,
        SubFunctionNotSupported = 0x12,
        IncorrectMessageLengthOrInvalidFormat = 0x13,
        ResponseTooLong = 0x14,
        // 0x15 to 0x20 are reserved by ISO14229-1:2020
        BusyRepeatRequest = 0x21,
        ConditionsNotCorrect = 0x22,
        RequestSequenceError = 0x24,
        NoResponseFromSubnetComponent = 0x25,
        FailurePreventsExecutionOfRequestedAction = 0x26,
        // 0x27 to 0x30 are reserved by ISO14229-1:2020
        RequestOutOfRange = 0x31,
        // 0x32 is reserved by ISO14229-1:2020
        SecurityAccessDenied = 0x33,
        AuthenticationRequired = 0x34,
        InvalidKey = 0x35,
        ExceedNumberOfAttempts = 0x36,
        RequiredTimeDelayNotExpired = 0x37,
        SecureDataTransmissionRequired = 0x38,
        SecureDataTransmissionNotAllowed = 0x39,
        SecureDataVerificationFailed = 0x3A,
        // 0x3B to 0x4F are reserved by ISO14229-1:2020
        CertficateVerificationFailedInvalidTimePeriod = 0x50,
        CertficateVerificationFailedInvalidSignature = 0x51,
        CertficateVerificationFailedInvalidChainOfTrust = 0x52,
        CertficateVerificationFailedInvalidType = 0x53,
        CertficateVerificationFailedInvalidFormat = 0x54,
        CertficateVerificationFailedInvalidContent = 0x55,
        CertficateVerificationFailedInvalidScope = 0x56,
        CertficateVerificationFailedInvalidCertificate = 0x57,
        OwnershipVerificationFailed = 0x58,
        ChallengeCalculationFailed = 0x59,
        SettingAccessRightsFailed = 0x5A,
        SessionKeyCreationOrDerivationFailed = 0x5B,
        ConfigurationDataUsageFailed = 0x5C,
        DeAuthenticationFailed = 0x5D,
        // 0x5E to 0x6F are reserved by ISO14229-1:2020
        UploadDownloadNotAccepted = 0x70,
        TransferDataSuspended = 0x71,
        GeneralProgrammingFailure = 0x72,
        WrongBlockSequenceCounter = 0x73,
        // 0x74 to 0x77 are reserved by ISO14229-1:2020
        RequestCorrectlyReceived_ResponsePending = 0x78,
        // 0x79 to 0x7D are reserved by ISO14229-1:2020
        SubFunctionNotSupportedInActiveSession = 0x7E,
        ServiceNotSupportedInActiveSession = 0x7F,
        // 0x80 is reserved by ISO14229-1:2020
        RpmTooHigh = 0x81,
        RpmTooLow = 0x82,
        EngineIsRunning = 0x83,
        EngineIsNotRunning = 0x84,
        EngineRunTimeTooLow = 0x85,
        TemperatureTooHigh = 0x86,
        TemperatureTooLow = 0x87,
        VehicleSpeedTooHigh = 0x88,
        VehicleSpeedTooLow = 0x89,
        ThrottlePedalTooHigh = 0x8A,
        ThrottlePedalTooLow = 0x8B,
        TransmissionRangeNotInNeutral = 0x8C,
        TransmissionRangeNotInGear = 0x8D,
        // 0x8E is reserved by ISO14229-1:2020
        BrakeSwitchNotClosed = 0x8F,
        ShifterLeverNotInPark = 0x90,
        TorqueConverterClutchLocked = 0x91,
        VoltageTooHigh = 0x92,
        VoltageTooLow = 0x93,
        ResourceTemporarilyNotAvailable = 0x94,
    };


    inline std::ostream &operator<<(std::ostream &os, const UdsResponseCode &code) {
        os << "UdsResponseCode(0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(code) << std::dec << ")";

        if (code == UdsResponseCode::OK) {
            os << " " << ansi::green << "OK" << ansi::reset;
        } else {
           os << " " << ansi::red << "NRC" << ansi::reset;
           switch(code) {
               case UdsResponseCode::GeneralReject:
                   os << " General Reject";
                   break;
               case UdsResponseCode::ServiceNotSupported:
                   os << " Service Not Supported";
                   break;
               case UdsResponseCode::SubFunctionNotSupported:
                   os << " SubFunction Not Supported";
                   break;
               case UdsResponseCode::IncorrectMessageLengthOrInvalidFormat:
                   os << " Incorrect Message Length Or Invalid Format";
                   break;
               case UdsResponseCode::ResponseTooLong:
                   os << " Response Too Long";
                   break;
               case UdsResponseCode::BusyRepeatRequest:
                   os << " Busy Repeat Request";
                   break;
               case UdsResponseCode::ConditionsNotCorrect:
                   os << " Conditions Not Correct";
                   break;
               case UdsResponseCode::RequestSequenceError:
                   os << " Request Sequence Error";
                   break;
                case UdsResponseCode::NoResponseFromSubnetComponent:
                   os << " No Response From Subnet Component";
                   break;
               case UdsResponseCode::FailurePreventsExecutionOfRequestedAction:
                   os << " Failure Prevents Execution Of Requested Action";
                   break;
               case UdsResponseCode::RequestOutOfRange:
                   os << " Request Out Of Range";
                   break;
               case UdsResponseCode::SecurityAccessDenied:
                   os << " Security Access Denied";
                   break;
               case UdsResponseCode::AuthenticationRequired:
                   os << " AuthenticationRequired";
                   break;
               case UdsResponseCode::InvalidKey:
                   os << " InvalidKey";
                   break;
               case UdsResponseCode::ExceedNumberOfAttempts:
                   os << " ExceedNumberOfAttempts";
                   break;
               case UdsResponseCode::RequiredTimeDelayNotExpired:
                   os << " RequiredTimeDelayNotExpired";
                   break;
               case UdsResponseCode::SecureDataTransmissionRequired:
                   os << " SecureDataTransmissionRequired";
                   break;
               case UdsResponseCode::SecureDataTransmissionNotAllowed:
                   os << " SecureDataTransmissionNotAllowed";
                   break;
               case UdsResponseCode::SecureDataVerificationFailed:
                   os << " SecureDataVerificationFailed";
                   break;
               case UdsResponseCode::UploadDownloadNotAccepted:
                   os << " UploadDownloadNotAccepted";
                   break;
               case UdsResponseCode::TransferDataSuspended:
                   os << " TransferDataSuspended";
                   break;
               case UdsResponseCode::GeneralProgrammingFailure:
                   os << " GeneralProgrammingFailure";
                   break;
               case UdsResponseCode::WrongBlockSequenceCounter:
                   os << " WrongBlockSequenceCounter";
                   break;
               case UdsResponseCode::RequestCorrectlyReceived_ResponsePending:
                   os << " RequestCorrectlyReceived_ResponsePending";
                   break;
               case UdsResponseCode::SubFunctionNotSupportedInActiveSession:
                   os << " SubFunctionNotSupportedInActiveSession";
                   break;
               case UdsResponseCode::ServiceNotSupportedInActiveSession:
                   os << " ServiceNotSupportedInActiveSession";
                   break;
               case UdsResponseCode::RpmTooHigh:
                   os << " RpmTooHigh";
                   break;
               case UdsResponseCode::RpmTooLow:
                   os << " RpmTooLow";
                   break;
                case UdsResponseCode::EngineIsRunning:
                   os << " EngineIsRunning";
                   break;
               case UdsResponseCode::EngineIsNotRunning:
                   os << " EngineIsNotRunning";
                   break;
               case UdsResponseCode::EngineRunTimeTooLow:
                   os << " EngineRunTimeTooLow";
                   break;
               case UdsResponseCode::TemperatureTooHigh:
                   os << " TemperatureTooHigh";
                   break;
               case UdsResponseCode::TemperatureTooLow:
                   os << " TemperatureTooLow";
                   break;
               case UdsResponseCode::VehicleSpeedTooHigh:
                   os << " VehicleSpeedTooHigh";
                   break;
               case UdsResponseCode::VehicleSpeedTooLow:
                   os << " VehicleSpeedTooLow";
                   break;
               case UdsResponseCode::ThrottlePedalTooHigh:
                   os << " ThrottlePedalTooHigh";
                   break;
               case UdsResponseCode::ThrottlePedalTooLow:
                   os << " ThrottlePedalTooLow";
                   break;
               case UdsResponseCode::TransmissionRangeNotInNeutral:
                   os << " TransmissionRangeNotInNeutral";
                   break;
               case UdsResponseCode::TransmissionRangeNotInGear:
                   os << " TransmissionRangeNotInGear";
                   break;
               case UdsResponseCode::BrakeSwitchNotClosed:
                   os << " BrakeSwitchNotClosed";
                   break;
               case UdsResponseCode::ShifterLeverNotInPark:
                   os << " ShifterLeverNotInPark";
                   break;
               case UdsResponseCode::TorqueConverterClutchLocked:
                   os << " TorqueConverterClutchLocked";
                   break;
               case UdsResponseCode::VoltageTooHigh:
                   os << " VoltageTooHigh";
                   break;
               case UdsResponseCode::VoltageTooLow:
                   os << " VoltageTooLow";
                   break;
               case UdsResponseCode::ResourceTemporarilyNotAvailable:
                   os << " ResourceTemporarilyNotAvailable";
                   break;
               default:
                   os << " UnknownNRC";
                   break;
           }
        }
        return os;
    }
} // namespace doip::uds

#endif /* UDSRESPONSE_H */

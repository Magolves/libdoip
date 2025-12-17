#include <doctest/doctest.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "DoIPCloseReason.h"
#include "DoIPDownstreamResult.h"
#include "DoIPFurtherAction.h"
#include "DoIPNegativeDiagnosticAck.h"
#include "DoIPPayloadType.h"
#include "DoIPServerState.h"
#include "DoIPDefaultConnection.h"

using namespace doip;

TYPE_TO_STRING_AS("Close reason", DoIPCloseReason);


TEST_SUITE("Overloaded Stream Operator Tests") {
    TEST_CASE("Test Stream Read/Write Operations") {
        // The payload identifiers are not sequential, so we list them explicitly
        std::vector<DoIPPayloadType> payloadTypes = {
            DoIPPayloadType::NegativeAck,
            DoIPPayloadType::VehicleIdentificationRequest,
            DoIPPayloadType::VehicleIdentificationRequestWithEid,
            DoIPPayloadType::VehicleIdentificationRequestWithVin,
            DoIPPayloadType::VehicleIdentificationResponse,
            DoIPPayloadType::RoutingActivationRequest,
            DoIPPayloadType::RoutingActivationResponse,
            DoIPPayloadType::AliveCheckRequest,
            DoIPPayloadType::AliveCheckResponse,
            DoIPPayloadType::EntityStatusRequest,
            DoIPPayloadType::EntityStatusResponse,
            DoIPPayloadType::DiagnosticPowerModeRequest,
            DoIPPayloadType::DiagnosticPowerModeResponse,
            DoIPPayloadType::DiagnosticMessage,
            DoIPPayloadType::DiagnosticMessageAck,
            DoIPPayloadType::DiagnosticMessageNegativeAck,
            DoIPPayloadType::PeriodicDiagnosticMessage};

        for (const auto &payloadType : payloadTypes) {
            std::stringstream ss;

            // Write to stream
            ss << payloadType;

            INFO(ss.str());

            // Read back from stream
            CHECK(ss.good());
            CHECK(ss.tellp() > 0);
        }
    }

    // Template test case for various enum types -> basically shots in the dark, assuming sequential types with less than 30 entries
    TEST_CASE_TEMPLATE("Stream Operator with Various Types", T, DoIPCloseReason, DoIPDownstreamResult, DoIPFurtherAction, DoIPNegativeDiagnosticAck, DoIPServerState, ConnectionTimers) {
        for (uint8_t i = 0; i < 30; ++i) {
            std::stringstream ss;
            T value = static_cast<T>(i);

            ss << value;

            INFO("Testing value: '" << value << "' with ordinal " << static_cast<int>(value));
            CHECK(ss.good());
            CHECK(ss.tellp() > 0);
        }
    }
}
#include "DoIPMessage.h"
#include "DoIPServer.h"
#include <doctest/doctest.h>
#include <sstream>

#include "doctest_aux.h"

using namespace doip;
using namespace std;

TEST_SUITE("VehicleIdentificationHandler") {
    struct VehicleIdentificationHandlerFixture {
        DoIpVin matchingVIN = DoIpVin("MatchingVin_12345");
        DoIpVin shortVIN = DoIpVin("shortVin");
        DoIpVin shortVINPadded = DoIpVin("shortVin000000000");
        DoIpEid EID = DoIpEid::Zero;
        DoIpGid GID = DoIpGid::Zero;
        DoIPFurtherAction furtherActionRequired = DoIPFurtherAction::NoFurtherAction;
        DoIPFurtherAction far_cs = DoIPFurtherAction::RoutingActivationForCentralSecurity;

        VehicleIdentificationHandlerFixture() {
            // Setup code here if needed
        }

        ~VehicleIdentificationHandlerFixture() {
            // Cleanup code here if needed
        }
    };

    /*
     * Checks if a VIN with 17 bytes matches correctly the input data
     */
    TEST_CASE_FIXTURE(VehicleIdentificationHandlerFixture, "VIN 17 Bytes") {
        DoIPMessage msg = message::makeVehicleIdentificationResponse(matchingVIN, ZERO_ADDRESS, EID, GID, furtherActionRequired);
        ByteArrayRef payload = msg.getPayload();
        ByteArray expected{
            // VIN (17 bytes) - now uppercase due to DoIpVin normalization
            'M', 'A', 'T', 'C', 'H', 'I', 'N', 'G', 'V', 'I', 'N', '_', '1', '2', '3', '4', '5',
            // Logical Address (2 bytes)
            0x00, 0x00,
            // Entity Type (6 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Group ID (6 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Further Action Required (1 byte)
            0x00,
            // Sync Status (1 byte)
            0x00};

        CHECK(msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse);
        CHECK(payload.second == expected.size());
        CHECK_BYTE_ARRAY_EQ(ByteArray(payload.first, payload.second), expected);
    }

    /*
     * Checks if a VIN < 17 bytes is padded correctly with zero bytes
     */
    TEST_CASE_FIXTURE(VehicleIdentificationHandlerFixture,
                      "VIN Less Than 17 Bytes") {

        DoIPMessage msg = message::makeVehicleIdentificationResponse(shortVIN, ZERO_ADDRESS, EID, GID, far_cs);
        ByteArrayRef payload = msg.getPayload();
        ByteArray expected{
            // VIN (17 bytes) - now uppercase due to DoIpVin normalization
            'S', 'H', 'O', 'R', 'T', 'V', 'I', 'N', '0', '0', '0', '0', '0', '0', '0', '0', '0',
            // Logical Address (2 bytes)
            0x00, 0x00,
            // Entity Type (6 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Group ID (6 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Further Action Required (1 byte)
            0x10,
            // Sync Status (1 byte)
            0x00};


        CHECK(msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse);
        CHECK(payload.second == expected.size());
        CHECK_BYTE_ARRAY_EQ(ByteArray(payload.first, payload.second), expected);
    }
}

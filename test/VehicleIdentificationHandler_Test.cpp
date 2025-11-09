#include "DoIPMessage.h"
#include "DoIPServer.h"
#include <doctest/doctest.h>
#include <stdint.h>
#include <string>

#include "doctest_aux.h"

using namespace doip;
using namespace std;

TEST_SUITE("VehicleIdentificationHandler") {
    struct VehicleIdentificationHandlerFixture {
        DoIPVIN matchingVIN = DoIPVIN("MatchingVin_12345");
        DoIPVIN shortVIN = DoIPVIN("shortVin");
        DoIPVIN shortVINPadded = DoIPVIN("shortVin000000000");
        DoIPEID EID = DoIPEID::Zero;
        DoIPGID GID = DoIPGID::Zero;
        DoIPFurtherAction far = DoIPFurtherAction::NoFurtherAction;
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
        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(matchingVIN, DoIPAddress::ZeroAddress, EID, GID, far);
        ByteArray payload = msg.getPayload();
        ByteArray expected{
            // VIN (17 bytes)
            'M', 'a', 't', 'c', 'h', 'i', 'n', 'g', 'V', 'i', 'n', '_', '1', '2', '3', '4', '5',
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
        CHECK(payload.size() == expected.size());
        CHECK_BYTE_ARRAY_EQ(payload, expected);
    }

    /*
     * Checks if a VIN < 17 bytes is padded correctly with zero bytes
     */
    TEST_CASE_FIXTURE(VehicleIdentificationHandlerFixture,
                      "VIN Less Than 17 Bytes") {

        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(shortVIN, DoIPAddress::ZeroAddress, EID, GID, far_cs);
        ByteArray payload = msg.getPayload();
        ByteArray expected{
            // VIN (17 bytes)
            's', 'h', 'o', 'r', 't', 'V', 'i', 'n', '0', '0', '0', '0', '0', '0', '0', '0', '0',
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
        CHECK(payload.size() == expected.size());
        CHECK_BYTE_ARRAY_EQ(payload, expected);
    }
}

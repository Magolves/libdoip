#include "DoIPServer.h"
#include "DoIPMessage.h"
#include "DoIPFurtherAction.h"
#include <doctest/doctest.h>
#include <stdint.h>
#include <string>

#include "doctest_aux.h"

using namespace doip;
using namespace std;

TEST_SUITE("DoIPServer Tests") {
    struct DoIPServerFixture {
        DoIPServer server;

        DoIPServerFixture() {
            // Setup code here if needed
        }

        ~DoIPServerFixture() {
            // Cleanup code here if needed
        }
    };

    /*
     * Test setting the VIN correctly
     */
    TEST_CASE_FIXTURE(DoIPServerFixture, "Set VIN Test") {
        std::string testVIN = "TESTVIN1234567890";
        server.setVIN(testVIN);
        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(server.getVIN(), DoIPAddress::ZeroAddress, server.getEID(), DoIPGID::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArray payload = msg.getPayload();

        // Check that the VIN in the payload matches the set VIN
        for (size_t i = 0; i < 17; ++i) {
            CHECK_MESSAGE(payload[i] == static_cast<uint8_t>(testVIN[i]), "VIN byte mismatch at index " << i << ": expected " << testVIN[i] << ", got " << +payload[i]);
        }
    }

    TEST_CASE_FIXTURE(DoIPServerFixture, "Set EID Test") {
        uint64_t testEID = 0x123456789ABC;
        server.setEID(testEID);
        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(server.getVIN(), DoIPAddress::ZeroAddress, server.getEID(), DoIPGID::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArray payload = msg.getPayload();

        // Check that the EID in the payload matches the set EID
        for (size_t i = 0; i < 6; ++i) {
            CHECK(payload[17 + 2 + i] == ((testEID >> (40 - i * 8)) & 0xFF));
        }
    }

    TEST_CASE_FIXTURE(DoIPServerFixture, "Set EID default") {
        bool result = server.setEIDdefault();
        CHECK(result == true);

        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(server.getVIN(), DoIPAddress::ZeroAddress, server.getEID(), DoIPGID::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArray payload = msg.getPayload();

        std::cerr << "EID set to: " << server.getEID().toHexString() << '\n';
        auto zeros = std::count_if(payload.begin() + 17 + 2, payload.begin() + 17 + 2 + 6, [](uint8_t byte) { return byte == 0; });
        CHECK(zeros < 6); // At least one byte should not be zero
    }
}
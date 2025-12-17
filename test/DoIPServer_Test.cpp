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
            CHECK(server.isRunning() == false);
            CHECK(server.getVin() == DoIpVin::Zero);
            CHECK(server.getEid() == DoIpEid::Zero);
            CHECK(server.getGid() == DoIpGid::Zero);
            CHECK(server.getLogicalGatewayAddress() == DoIPAddress(0x0028));
            CHECK(server.getClientIp() == "");
            CHECK(server.getClientPort() == 0);
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
        server.setVin(testVIN);
        DoIPMessage msg = message::makeVehicleIdentificationResponse(server.getVin(), ZERO_ADDRESS, server.getEid(), DoIpGid::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArrayRef payload = msg.getPayload();

        // Check that the VIN in the payload matches the set VIN
        for (size_t i = 0; i < 17; ++i) {
            CHECK_MESSAGE(payload.first[i] == static_cast<uint8_t>(testVIN[i]), "VIN byte mismatch at index " << i << ": expected " << testVIN[i] << ", got " << +payload.first[i]);
        }
    }

    TEST_CASE_FIXTURE(DoIPServerFixture, "Set EID Test") {
        uint64_t testEID = 0x123456789ABC;
        server.setEid(testEID);
        DoIPMessage msg = message::makeVehicleIdentificationResponse(server.getVin(), ZERO_ADDRESS, server.getEid(), DoIpGid::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArrayRef payload = msg.getPayload();

        // Check that the EID in the payload matches the set EID
        for (size_t i = 0; i < 6; ++i) {
            CHECK(payload.first[17 + 2 + i] == ((testEID >> (40 - i * 8)) & 0xFF));
        }
    }

    TEST_CASE_FIXTURE(DoIPServerFixture, "Set EID default") {
        bool result = server.setDefaultEid();
        CHECK(result == true);

        DoIPMessage msg = message::makeVehicleIdentificationResponse(server.getVin(), ZERO_ADDRESS, server.getEid(), DoIpGid::Zero, DoIPFurtherAction::NoFurtherAction);
        ByteArrayRef payload = msg.getPayload();

        std::cerr << "EID set to: " << server.getEid().toHexString() << '\n';
        auto zeros = std::count_if(payload.first + 17 + 2, payload.first + 17 + 2 + 6, [](uint8_t byte) { return byte == 0; });
        CHECK(zeros < 6); // At least one byte should not be zero
    }
}
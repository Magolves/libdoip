#include "MacAddress.h"
#include <doctest/doctest.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../doctest_aux.h"
#include "uds/UdsMock.h"
#include "uds/UdsMockProvider.h"

using namespace doip;
using namespace doip::uds;

TEST_SUITE("UdsMock") {
    struct UdsFixture {
        UdsMock udsMock;

        UdsFixture() {
            // Setup code here if needed
        }

        ~UdsFixture() {
            // Cleanup code here if needed
        }
    };

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock handles invalid service id") {
        ByteArray request = {0x00, 0x00}; // Invalid UDS service ID
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        ByteArray expectedResponse = {0x7F, 0x00, 0x11};

        INFO(response);
        CHECK(response == expectedResponse);
    }


    TEST_CASE_FIXTURE(UdsFixture, "UdsMock default behavior returns ServiceNotSupported") {
        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected negative response: 0x7F, 0x10, NRC for ServiceNotSupported (0x11)
        ByteArray expectedResponse = {0x7F, 0x10, 0x11};

        INFO(response);
        CHECK(response == expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock custom handler returns positive response") {
        udsMock.registerService(uds::UdsService::DiagnosticSessionControl,
                                [](const ByteArray &request) {
                                    // Custom handler that returns positive response
                                    return std::make_pair(uds::UdsResponseCode::OK, ByteArray{request[1], 1, 2, 3, 4});
                                });

        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x50, 0x01, 0x01, 0x02, 0x03, 0x04
        ByteArray expectedResponse = {0x50, 0x01, 0x01, 0x02, 0x03, 0x04};

        INFO(response);
        CHECK(response == expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock custom RDBI handler returns positive response") {
        udsMock.registerService(UdsService::ReadDataByIdentifier,
                                [](const ByteArray &request) {
                                    // Extract DID from request
                                    if (request.size() < 3) {
                                        return std::make_pair(uds::UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, ByteArray{});
                                    }
                                    uint16_t did = request.readU16BE(1);
                                    // Custom handler that returns positive response with dummy data
                                    ByteArray responseData;
                                    responseData.writeU16BE(did); // Echo back the DID
                                    responseData.push_back(0x12); // Dummy data byte 1
                                    responseData.push_back(0x34); // Dummy data byte 2
                                    return std::make_pair(uds::UdsResponseCode::OK, responseData);
                                });
        ByteArray request = {0x22, 0x01, 0x02}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x50, 0x01
        ByteArray expectedResponse = {0x62, 0x01, 0x02, 0x12, 0x34};

        INFO(response);
        CHECK_BYTE_ARRAY_EQ(response, expectedResponse);

        // bad request
        request = {0x22, 0x01}; // Example UDS request (Diagnostic Session Control)
        response = udsMock.handleDiagnosticRequest(request);
        expectedResponse = {0x7f, 0x22, 0x13};

        INFO(response);
        CHECK_BYTE_ARRAY_EQ(response, expectedResponse);
    }

     TEST_CASE_FIXTURE(UdsFixture, "UdsMock custom RDBI handler returns positive response") {
        UdsMockProvider udsProvider;
        // TODO: implement test cases for UdsMockProvider

     }
}

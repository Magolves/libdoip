#include <doctest/doctest.h>
#include "MacAddress.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sstream>

#include "uds/UdsMock.h"

using namespace doip;

TEST_SUITE("UdsMock") {

    TEST_CASE("UdsMock default behavior returns ServiceNotSupported") {
        uds::UdsMock udsMock;

        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected negative response: 0x7F, 0x10, NRC for ServiceNotSupported (0x11)
        ByteArray expectedResponse = {0x7F, 0x10, 0x11};

        CHECK(response == expectedResponse);
    }

    TEST_CASE("UdsMock custom handler returns positive response") {
        uds::UdsMock udsMock([](const ByteArray &request) {
            // Custom handler that always returns OK
            return std::make_pair(uds::UdsResponseCode::OK, ByteArray{request[1]}); // Positive response
        });

        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x50, 0x01
        ByteArray expectedResponse = {0x50, 0x01};

        CHECK(response == expectedResponse);
    }
}

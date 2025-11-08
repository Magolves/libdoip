#include <doctest/doctest.h>
#include "ByteArray.h"
#include <sstream>
#include <algorithm>

using namespace doip;

TEST_SUITE("ByteArray Stream Operator") {

    TEST_CASE("Empty ByteArray") {
        ByteArray arr;
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "");
    }

    TEST_CASE("Single byte") {
        ByteArray arr = {0x42};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "42");
    }

    TEST_CASE("Two bytes") {
        ByteArray arr = {0x01, 0x02};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "01.02");
    }

    TEST_CASE("Multiple bytes with leading zeros") {
        ByteArray arr = {0x00, 0x01, 0x0A, 0x10};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "00.01.0A.10");
    }

    TEST_CASE("Full byte range") {
        ByteArray arr = {0x00, 0x7F, 0x80, 0xFF};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "00.7F.80.FF");
    }

    TEST_CASE("DoIP protocol version and payload type") {
        ByteArray arr = {0x02, 0xFD, 0x80, 0x01};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "02.FD.80.01");
    }

    TEST_CASE("ASCII characters as hex") {
        ByteArray arr = {'H', 'e', 'l', 'l', 'o'};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "48.65.6C.6C.6F");
    }

    TEST_CASE("DoIP header example") {
        ByteArray header = {
            0x02, 0xFD,           // Protocol version
            0x80, 0x01,           // Payload type
            0x00, 0x00, 0x00, 0x04 // Payload length
        };
        std::ostringstream oss;

        oss << header;

        CHECK(oss.str() == "02.FD.80.01.00.00.00.04");
    }

    TEST_CASE("Stream state preservation") {
        ByteArray arr = {0xAB, 0xCD};
        std::ostringstream oss;

        // Set initial state to decimal
        oss << std::dec;
        oss << 123 << " ";

        // Print ByteArray (should use hex)
        oss << arr;

        // After ByteArray, stream should return to decimal
        oss << " " << 456;

        CHECK(oss.str() == "123 AB.CD 456");
    }

    TEST_CASE("Multiple ByteArrays in sequence") {
        ByteArray arr1 = {0x01, 0x02};
        ByteArray arr2 = {0xAA, 0xBB};
        std::ostringstream oss;

        oss << arr1 << " " << arr2;

        CHECK(oss.str() == "01.02 AA.BB");
    }

    TEST_CASE("Integration with other stream operations") {
        ByteArray arr = {0x12, 0x34, 0x56};
        std::ostringstream oss;

        oss << "Data: " << arr << " (3 bytes)";

        CHECK(oss.str() == "Data: 12.34.56 (3 bytes)");
    }

    TEST_CASE("Large ByteArray") {
        ByteArray arr;
        for (int i = 0; i < 256; ++i) {
            arr.push_back(static_cast<uint8_t>(i));
        }

        std::ostringstream oss;
        oss << arr;

        std::string result = oss.str();

        // Check first few bytes
        CHECK(result.substr(0, 8) == "00.01.02");

        // Check last few bytes
        CHECK(result.substr(result.length() - 8) == "FD.FE.FF");

        // Count dots (should be 255 for 256 bytes)
        size_t dot_count = static_cast<size_t>(std::count(result.begin(), result.end(), '.'));
        CHECK(dot_count == 255);
    }

    TEST_CASE("Lowercase hex digits are uppercase") {
        ByteArray arr = {0xab, 0xcd, 0xef};
        std::ostringstream oss;

        oss << arr;

        CHECK(oss.str() == "AB.CD.EF");
    }

    TEST_CASE("Can be used in CHECK messages") {
        ByteArray expected = {0x01, 0x02, 0x03};
        ByteArray actual   = {0x01, 0x02, 0x03};

        std::ostringstream oss_exp, oss_act;
        oss_exp << expected;
        oss_act << actual;

        CHECK(oss_exp.str() == oss_act.str());
        CHECK(oss_exp.str() == "01.02.03");
    }

    TEST_CASE("VIN as ByteArray") {
        ByteArray vin = {'W', 'V', 'W', 'Z', 'Z', 'Z', '1', 'J', 'Z', 'Y', 'W', '1', '2', '3', '4', '5', '6'};
        std::ostringstream oss;

        oss << vin;

        CHECK(oss.str() == "57.56.57.5A.5A.5A.31.4A.5A.59.57.31.32.33.34.35.36");
    }

    TEST_CASE("EID/GID size (6 bytes)") {
        ByteArray eid = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        std::ostringstream oss;

        oss << eid;

        CHECK(oss.str() == "00.11.22.33.44.55");
    }
}

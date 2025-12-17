#include <doctest/doctest.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "DoIPAddress.h"
#include "DoIPConnection.h"

using namespace doip;

/**
 * @brief Miscellaneous tests for various components where a dedicated module is not justified
 *
 */

TEST_SUITE("DoIPAddress") {
    TEST_CASE("Zero address") {
        DoIPAddress zeroAddr = readAddressFrom(nullptr, 0);
        CHECK(zeroAddr == ZERO_ADDRESS);
    }

    TEST_CASE("Valid source address") {
        uint8_t validData[] = { 0xE0, 0x10 }; // 0xE010 is a valid source address
        uint8_t invalidData[] = { 0xD0, 0x10 }; // 0xD010 is NOT a valid source address

        CHECK(isValidSourceAddress(validData, 0) == true);
        CHECK(isValidSourceAddress(invalidData, 0) == false);
    }
}

TEST_SUITE("DoIPConnection") {
    TEST_CASE("Connection Initialization") {

        DoIPConnection conn(0, std::make_unique<DefaultDoIPServerModel>());
        CHECK(conn.isSocketActive() == false);
    }
}
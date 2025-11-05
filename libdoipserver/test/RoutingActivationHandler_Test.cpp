#include "RoutingActivationHandler.h"
#include <stdint.h>
#include <doctest/doctest.h>
#include <array>

using namespace doip;

TEST_SUITE("RoutingActivation") {
	struct RoutingActivationFixture {
		std::array<uint8_t, 15> request;

		RoutingActivationFixture() : request{{
			0x01, 0xFE, 0x00, 0x05, 0x00,
			0x00, 0x00, 0x07, 0x0F, 0x12,
			0x00, 0x00, 0x00, 0x00, 0x00
		}} {}

		uint8_t* data() { return request.data(); }
		const uint8_t* data() const { return request.data(); }
	};

	/*
	* Checks if parsing a valid address will be found in the list of valid addresses
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid DoIPAddress Parsing") {
		uint16_t expectedValue = 3858; // 0x0F12
		DoIPAddress address(data(), 8);

		CHECK_MESSAGE(address.as_uint16() == expectedValue, "Converting address to uint failed");

		CHECK(address.isValidSourceAddress() == true);
		CHECK_MESSAGE(address.isValidSourceAddress(), "Didn't found address");
	}

	/*
	* Checks if parsing a wrong address leads to the correct negative response code (0x00)
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Wrong DoIPAddress Parsing") {
		//Set wrong address in received message
		request[8] = 0x0D;
		request[9] = 0x00;
		uint8_t result = parseRoutingActivation(data());
		CHECK(result == 0x00);
		CHECK_MESSAGE(result == 0x00, "Wrong address doesn't return correct response code");
	}

	/*
	* Checks if parsing a unknown activation type leads to the correct negative response code (0x06)
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Unknown Activation Type") {
		//Set wrong activation type
		//request[10] = 0xFF; ?? ow: is this correct?
		request[2] = 0xFF;
		uint8_t result = parseRoutingActivation(data());
		CHECK(result == 0x06);
	}

	/*
	* Checks if a valid routing activation request leads to the correct routing activation code (0x10);
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid Request") {
		uint8_t result = parseRoutingActivation(data());
		CHECK(result == 0x10);
	}
}

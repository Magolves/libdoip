#include "RoutingActivationHandler.h"
#include <stdint.h>
#include <doctest/doctest.h>

TEST_SUITE("RoutingActivation") {
	struct RoutingActivationFixture {
		uint8_t* request;

		RoutingActivationFixture() {
			request = new uint8_t[15];
			request[0] = 0x01;
			request[1] = 0xFE;
			request[2] = 0x00;
			request[3] = 0x05;
			request[4] = 0x00;
			request[5] = 0x00;
			request[6] = 0x00;
			request[7] = 0x07;
			request[8] = 0x0F;
			request[9] = 0x12;
			request[10] = 0x00;
			request[11] = 0x00;
			request[12] = 0x00;
			request[13] = 0x00;
			request[14] = 0x00;
		}

		~RoutingActivationFixture() {
			delete[] request;
		}
	};

	/*
	* Checks if parsing a valid address will be found in the list of valid addresses
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid DoIPAddress Parsing") {
		uint16_t expectedValue = 3858; // 0x0F12
		DoIPAddress address(request, 8);

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
		uint8_t result = parseRoutingActivation(request);
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
		uint8_t result = parseRoutingActivation(request);
		CHECK(result == 0x06);
	}

	/*
	* Checks if a valid routing activation request leads to the correct routing activation code (0x10);
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid Request") {
		uint8_t result = parseRoutingActivation(request);
		CHECK(result == 0x10);
	}
}

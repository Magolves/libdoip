#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "RoutingActivationHandler.h"
#include <stdint.h>

TEST_SUITE("RoutingActivation") {
	struct RoutingActivationFixture {
		unsigned char* request;

		RoutingActivationFixture() {
			request = new unsigned char[15];
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
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid Address Parsing") {
		uint32_t expectedValue = 3858;
		uint32_t address = 0;
		address |= (uint32_t)request[8] << 8;
		address |= (uint32_t)request[9];
		CHECK_EQ(address, expectedValue);
		CHECK_MESSAGE(address == expectedValue, "Converting address to uint failed");

		bool result = checkSourceAddress(address);
		CHECK(result);
		CHECK_MESSAGE(result, "Didnt found address");
	}

	/*
	* Checks if parsing a wrong address leads to the correct negative response code (0x00)
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Wrong Address Parsing") {
		//Set wrong address in received message
		request[8] = 0x0D;
		request[9] = 0x00;
		unsigned char result = parseRoutingActivation(request);
		CHECK_EQ(result, 0x00);
		CHECK_MESSAGE(result == 0x00, "Wrong address doesn't return correct response code");
	}

	/*
	* Checks if parsing a unknown activation type leads to the correct negative response code (0x06)
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Unknown Activation Type") {
		//Set wrong activation type
		request[10] = 0xFF;
		unsigned char result = parseRoutingActivation(request);
		CHECK_EQ(result, 0x06);
	}

	/*
	* Checks if a valid routing activation request leads to the correct routing activation code (0x10);
	*/
	TEST_CASE_FIXTURE(RoutingActivationFixture, "Valid Request") {
		unsigned char result = parseRoutingActivation(request);
		CHECK_EQ(result, 0x10);
	}
}

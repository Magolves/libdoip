#include <doctest/doctest.h>
#include "DoIPGenericHeaderHandler.h"
#include <array>

using namespace doip;

TEST_SUITE("GenericHeader") {
	struct GenericHeaderFixture {
		std::array<uint8_t, 15> request;

		GenericHeaderFixture() : request{{
			0x01, 0xFE, 0x00, 0x05, 0x00,
			0x00, 0x00, 0x07, 0x0F, 0x12,
			0x00, 0x00, 0x00, 0x00, 0x00
		}} {}

		uint8_t* data() { return request.data(); }
		const uint8_t* data() const { return request.data(); }
	};

	/*
	* Checks if a wrong synchronization pattern leads to the correct response type and NACK code (0x00)
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Wrong Synchronization Pattern") {
		//Set wrong inverse protocol version
		request[1] = 0x33;
		GenericHeaderAction action = parseGenericHeader(data(), 15);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_MESSAGE(action.type == PayloadType::NEGATIVEACK, "returned payload type is wrong");
		CHECK_EQ(action.value, 0x00);
		CHECK_MESSAGE(action.value == 0x00, "returned NACK code is wrong");

		//Set currently not supported protocol version
		request[1] = 0xFE;
		request[0] = 0x04;
		action = parseGenericHeader(data(), 15);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_MESSAGE(action.type == PayloadType::NEGATIVEACK, "returned payload type is wrong");
		CHECK_EQ(action.value, 0x00);
		CHECK_MESSAGE(action.value == 0x00, "returned NACK code is wrong");
	}

	/*
	* Checks if a unknown payload type leads to the correct response type and NACK code (0x01)
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Unknown Payload Type") {
		//Set unknown payload type (0x0010)
		request[3] = 0x10;
		GenericHeaderAction action = parseGenericHeader(data(), 15);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_EQ(action.value, 0x01);
	}

	/*
	* Checks if a known payload type in the request leads to the correct payload type in action
	* Checks Routing Activation Request type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Known Payload Type - Routing Activation Request") {
		GenericHeaderAction action = parseGenericHeader(data(), 15);
		CHECK_EQ(action.type, PayloadType::ROUTINGACTIVATIONREQUEST);
	}

	/*
	* Checks if a known payload type in the request leads to the correct payload type in action
	* Checks Vehicle Identification Request type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Known Payload Type - Vehicle Identification Request") {
		//change payload type
		request[2] = 0x00;
		request[3] = 0x01;

		//change payload length
		request[7] = 0x00;

		GenericHeaderAction action = parseGenericHeader(data(), 8);	//VehidleIdentificationRequest length only 8
		CHECK_EQ(action.type, PayloadType::VEHICLEIDENTREQUEST);
	}

	/*
	* Checks if a known payload type in the request leads to the correct payload type in action
	* Checks Diagnostic Message type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Known Payload Type - Diagnostic Message") {
		request[2] = 0x80;
		request[3] = 0x01;
		GenericHeaderAction action = parseGenericHeader(data(), 15);
		CHECK_EQ(action.type, PayloadType::DIAGNOSTICMESSAGE);
	}

	/*
	* Checks if a wrong routing activation payload length return the correct response type and NACK code (0x04)
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Wrong Routing Activation Length") {
		request[7] = 0x08; // Use invalid routing activation payload length 8
		GenericHeaderAction action = parseGenericHeader(data(), 20);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_EQ(action.value, 0x04);
	}

	/*
	* Checks if a valid generic header leads to the correct response type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Valid Generic Header") {
		GenericHeaderAction action = parseGenericHeader(data(), 15);
		CHECK_NE(action.type, PayloadType::NEGATIVEACK);
	}
}

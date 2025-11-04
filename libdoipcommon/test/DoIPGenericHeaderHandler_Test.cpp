#include <doctest/doctest.h>
#include "DoIPGenericHeaderHandler.h"

using namespace doip;

TEST_SUITE("GenericHeader") {
	struct GenericHeaderFixture {
		uint8_t* request;

		GenericHeaderFixture() {
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

		~GenericHeaderFixture() {
			delete[] request;
		}
	};

	/*
	* Checks if a wrong synchronization pattern leads to the correct response type and NACK code (0x00)
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Wrong Synchronization Pattern") {
		//Set wrong inverse protocol version
		request[1] = 0x33;
		GenericHeaderAction action = parseGenericHeader(request, 15);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_MESSAGE(action.type == PayloadType::NEGATIVEACK, "returned payload type is wrong");
		CHECK_EQ(action.value, 0x00);
		CHECK_MESSAGE(action.value == 0x00, "returned NACK code is wrong");

		//Set currently not supported protocol version
		request[1] = 0xFE;
		request[0] = 0x04;
		action = parseGenericHeader(request, 15);
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
		GenericHeaderAction action = parseGenericHeader(request, 15);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_EQ(action.value, 0x01);
	}

	/*
	* Checks if a known payload type in the request leads to the correct payload type in action
	* Checks Routing Activation Request type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Known Payload Type - Routing Activation Request") {
		GenericHeaderAction action = parseGenericHeader(request, 15);
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

		GenericHeaderAction action = parseGenericHeader(request, 8);	//VehidleIdentificationRequest length only 8
		CHECK_EQ(action.type, PayloadType::VEHICLEIDENTREQUEST);
	}

	/*
	* Checks if a known payload type in the request leads to the correct payload type in action
	* Checks Diagnostic Message type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Known Payload Type - Diagnostic Message") {
		request[2] = 0x80;
		request[3] = 0x01;
		GenericHeaderAction action = parseGenericHeader(request, 15);
		CHECK_EQ(action.type, PayloadType::DIAGNOSTICMESSAGE);
	}

	/*
	* Checks if a wrong routing activation payload length return the correct response type and NACK code (0x04)
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Wrong Routing Activation Length") {
		request[7] = 0x08; // Use invalid routing activation payload length 8
		GenericHeaderAction action = parseGenericHeader(request, 20);
		CHECK_EQ(action.type, PayloadType::NEGATIVEACK);
		CHECK_EQ(action.value, 0x04);
	}

	/*
	* Checks if a valid generic header leads to the correct response type
	*/
	TEST_CASE_FIXTURE(GenericHeaderFixture, "Valid Generic Header") {
		GenericHeaderAction action = parseGenericHeader(request, 15);
		CHECK_NE(action.type, PayloadType::NEGATIVEACK);
	}
}

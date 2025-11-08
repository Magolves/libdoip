#include <doctest/doctest.h>
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include <sstream>

using namespace doip;

TEST_SUITE("DoIPPayloadType Stream Operator") {

    TEST_CASE("NegativeAck") {
        std::ostringstream oss;
        oss << DoIPPayloadType::NegativeAck;
        CHECK(oss.str() == "NegativeAck (0x0000)");
    }

    TEST_CASE("VehicleIdentificationRequest") {
        std::ostringstream oss;
        oss << DoIPPayloadType::VehicleIdentificationRequest;
        CHECK(oss.str() == "VehicleIdentificationRequest (0x0001)");
    }

    TEST_CASE("VehicleIdentificationResponse") {
        std::ostringstream oss;
        oss << DoIPPayloadType::VehicleIdentificationResponse;
        CHECK(oss.str() == "VehicleIdentificationResponse (0x0004)");
    }

    TEST_CASE("RoutingActivationRequest") {
        std::ostringstream oss;
        oss << DoIPPayloadType::RoutingActivationRequest;
        CHECK(oss.str() == "RoutingActivationRequest (0x0005)");
    }

    TEST_CASE("AliveCheckRequest") {
        std::ostringstream oss;
        oss << DoIPPayloadType::AliveCheckRequest;
        CHECK(oss.str() == "AliveCheckRequest (0x0007)");
    }

    TEST_CASE("DiagnosticMessage") {
        std::ostringstream oss;
        oss << DoIPPayloadType::DiagnosticMessage;
        CHECK(oss.str() == "DiagnosticMessage (0x8001)");
    }

    TEST_CASE("DiagnosticMessageAck") {
        std::ostringstream oss;
        oss << DoIPPayloadType::DiagnosticMessageAck;
        CHECK(oss.str() == "DiagnosticMessageAck (0x8002)");
    }

    TEST_CASE("DiagnosticMessageNegativeAck") {
        std::ostringstream oss;
        oss << DoIPPayloadType::DiagnosticMessageNegativeAck;
        CHECK(oss.str() == "DiagnosticMessageNegativeAck (0x8003)");
    }

    TEST_CASE("EntityStatusRequest") {
        std::ostringstream oss;
        oss << DoIPPayloadType::EntityStatusRequest;
        CHECK(oss.str() == "EntityStatusRequest (0x4001)");
    }

    TEST_CASE("Use in logging context") {
        DoIPPayloadType type = DoIPPayloadType::RoutingActivationResponse;
        std::ostringstream oss;
        oss << "Received message type: " << type;
        CHECK(oss.str() == "Received message type: RoutingActivationResponse (0x0006)");
    }
}

TEST_SUITE("DoIPMessage Stream Operator") {

    TEST_CASE("Vehicle Identification Request (empty payload)") {
        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationRequest();
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("Protocol: 0x04") != std::string::npos);
        CHECK(result.find("VehicleIdentificationRequest") != std::string::npos);
        CHECK(result.find("Size: 0 bytes") != std::string::npos);
    }

    TEST_CASE("Diagnostic Message with payload") {
        DoIPAddress source(0x12, 0x34);
        DoIPAddress target(0x56, 0x78);
        ByteArray diagnostic_data = {0x3E, 0x01}; // Tester Present

        DoIPMessage msg = DoIPMessage::makeDiagnosticMessage(source, target, diagnostic_data);
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("Protocol: 0x04") != std::string::npos);
        CHECK(result.find("DiagnosticMessage") != std::string::npos);
        CHECK(result.find("Size: 6 bytes") != std::string::npos);
        CHECK(result.find("12.34.56.78.3E.01") != std::string::npos);
    }

    TEST_CASE("Vehicle Identification Response") {
        DoIPVIN vin("WVWZZZ1JZYW123456");
        DoIPAddress logicalAddr(0x0E, 0x80);
        DoIPEID eid = DoIPEID::Zero;
        DoIPGID gid = DoIPGID::Zero;

        DoIPMessage msg = DoIPMessage::makeVehicleIdentificationResponse(
            vin, logicalAddr, eid, gid, DoIPFurtherAction::NoFurtherAction);

        std::ostringstream oss;
        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("Protocol: 0x04") != std::string::npos);
        CHECK(result.find("VehicleIdentificationResponse") != std::string::npos);
        CHECK(result.find("Size: 33 bytes") != std::string::npos);
    }

    TEST_CASE("Alive Check Request") {
        DoIPMessage msg = DoIPMessage::makeAliveCheckRequest();
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("AliveCheckRequest") != std::string::npos);
        CHECK(result.find("Size: 0 bytes") != std::string::npos);
    }

    TEST_CASE("Alive Check Response") {
        DoIPAddress addr(0x10, 0x00);
        DoIPMessage msg = DoIPMessage::makeAliveCheckResponse(addr);
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("AliveCheckResponse") != std::string::npos);
        CHECK(result.find("Size: 2 bytes") != std::string::npos);
        CHECK(result.find("10.00") != std::string::npos);
    }

    TEST_CASE("Negative ACK Message") {
        DoIPMessage msg = DoIPMessage::makeNegativeAckMessage(DoIPNegativeAck::InvalidPayloadLength);
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("NegativeAck") != std::string::npos);
        CHECK(result.find("Size: 1 bytes") != std::string::npos);
    }

    TEST_CASE("Diagnostic Positive Response") {
        DoIPAddress source(0xAB, 0xCD);
        DoIPAddress target(0xEF, 0x01);
        ByteArray response_data = {0x7E, 0x01}; // Positive response to Tester Present

        DoIPMessage msg = DoIPMessage::makeDiagnosticPositiveResponse(source, target, response_data);
        std::ostringstream oss;

        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("DiagnosticMessageAck") != std::string::npos);
        CHECK(result.find("Size: 7 bytes") != std::string::npos);
    }

    TEST_CASE("Diagnostic Negative Response") {
        DoIPAddress source(0x11, 0x11);
        DoIPAddress target(0x22, 0x22);
        ByteArray response_data = {0x7F, 0x3E, 0x78}; // NRC 0x78

        DoIPMessage msg = DoIPMessage::makeDiagnosticNegativeResponse(
            source, target, DoIPNegativeDiagnosticAck::InvalidSourceAddress, response_data);

        std::ostringstream oss;
        oss << msg;

        std::string result = oss.str();
        CHECK(result.find("DiagnosticMessageNegativeAck") != std::string::npos);
        CHECK(result.find("Size: 8 bytes") != std::string::npos);
    }

    TEST_CASE("Multiple messages in sequence") {
        DoIPMessage msg1 = DoIPMessage::makeVehicleIdentificationRequest();
        DoIPMessage msg2 = DoIPMessage::makeAliveCheckRequest();

        std::ostringstream oss;
        oss << "Message 1: " << msg1 << "\n";
        oss << "Message 2: " << msg2;

        std::string result = oss.str();
        CHECK(result.find("Message 1:") != std::string::npos);
        CHECK(result.find("Message 2:") != std::string::npos);
        CHECK(result.find("VehicleIdentificationRequest") != std::string::npos);
        CHECK(result.find("AliveCheckRequest") != std::string::npos);
    }

    TEST_CASE("Stream state preservation") {
        DoIPMessage msg = DoIPMessage::makeAliveCheckRequest();
        std::ostringstream oss;

        oss << std::dec << 123 << " ";
        oss << msg;
        oss << " " << 456;

        std::string result = oss.str();
        CHECK(result.find("123") != std::string::npos);
        CHECK(result.find("456") != std::string::npos);
    }
}

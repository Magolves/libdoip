#include <doctest/doctest.h>

#include "DoIPMessage.h"

using namespace doip;

TEST_SUITE("DoIPMessage") {
    TEST_CASE("Message assembly") {
        DoIPMessage msg(DoIPPayloadType::AliveCheckRequest, {0x01, 0x02});
        ByteArray expected{
            0x04, 0xfb,             // protocol version + inv
            0x00, 0x07,             // payload type
            0x00, 0x00, 0x00, 0x02, // payload length
            0x01, 0x02              // payload
        };

        CHECK(msg.getPayloadSize() == 2);
        CHECK(msg.getMessageSize() == 10);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckRequest);

        const auto bytes = msg.toBytes();

        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeNegativeAckMessage") {
        DoIPMessage msg = DoIPMessage::makeNegativeAckMessage(DoIPNegativeAck::InvalidPayloadLength);
        ByteArray expected{
            0x04, 0xfb,             // protocol version + inv
            0x00, 0x00,             // payload type
            0x00, 0x00, 0x00, 0x01, // payload length
            0x04                    // payload
        };

        CHECK(msg.getPayloadSize() == 1);
        CHECK(msg.getMessageSize() == 9);
        CHECK(msg.getPayloadType() == DoIPPayloadType::NegativeAck);
    }

    TEST_CASE("Message factory - makeDiagnosticMessage") {
        DoIPMessage msg = DoIPMessage::makeDiagnosticMessage(DoIPAddress(0xca, 0xfe), DoIPAddress(0xba, 0xbe), {0xde, 0xad, 0xbe, 0xef});
        ByteArray expected{
            0x04, 0xfb,             // protocol version + inv
            0x80, 0x01,             // payload type
            0x00, 0x00, 0x00, 0x08, // payload length
            0xca, 0xfe,             // sa
            0xba, 0xbe,             // ta
            0xde, 0xad, 0xbe, 0xef  // payload
        };

        CHECK(msg.getPayloadSize() == 8);
        CHECK(msg.getMessageSize() == 16);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessage);

        const auto bytes = msg.toBytes();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeDiagnosticPositiveResponse") {
        DoIPMessage msg = DoIPMessage::makeDiagnosticPositiveResponse(DoIPAddress(0xca, 0xfe), DoIPAddress(0xba, 0xbe), {0xde, 0xad, 0xbe, 0xef});
        ByteArray expected{
            0x04, 0xfb,             // protocol version + inv
            0x80, 0x02,             // payload tyByteArray raw = DoIPMessage::makeAliveCheckResponse(DoIPAddress(0xa0, 0xb0));pe
            0x00, 0x00, 0x00, 0x09, // payload length
            0xca, 0xfe,             // sa
            0xba, 0xbe,             // ta
            0x00,                   // positive response
            0xde, 0xad, 0xbe, 0xef  // payload
        };

        CHECK(msg.getPayloadSize() == 9);
        CHECK(msg.getMessageSize() == 17);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessageAck);

        const auto bytes = msg.toBytes();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeDiagnosticNegativeResponse") {
        DoIPMessage msg = DoIPMessage::makeDiagnosticNegativeResponse(
            DoIPAddress(0xca, 0xfe),
            DoIPAddress(0xba, 0xbe),
            DoIPNegativeDiagnosticAck::TargetBusy,
            {0xde, 0xad, 0xbe, 0xef});

        ByteArray expected{
            0x04, 0xfb, // protocol version + inv
            0x80, 0x03,                      // payload type
            0x00, 0x00, 0x00, 0x09,          // payload length
            0xca, 0xfe,                      // sa
            0xba, 0xbe,                      // ta
            0x09,                            // negative response code
            0xde, 0xad, 0xbe, 0xef           // payload
        };

        CHECK(msg.getPayloadSize() == 9);
        CHECK(msg.getMessageSize() == 17);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessageNegativeAck);

        const auto bytes = msg.toBytes();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeAliveCheckRequest") {
        DoIPMessage msg = DoIPMessage::makeAliveCheckRequest();

        CHECK(msg.getPayloadSize() == 0);
        CHECK(msg.getMessageSize() == 8);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckRequest);
    }

    TEST_CASE("Message factory - makeAliveCheckResponse") {
        DoIPMessage msg = DoIPMessage::makeAliveCheckResponse(DoIPAddress(0xa0, 0xb0));
        ByteArray expected{
            0x04, 0xfb, // protocol version + inv
            0x00, 0x08,                      // payload type
            0x00, 0x00, 0x00, 0x02,          // payload length
            0xa0, 0xb0,                      // sa
        };


        CHECK(msg.getPayloadSize() == 2);
        CHECK(msg.getMessageSize() == 10);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckResponse);

        const auto bytes = msg.toBytes();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Init from raw bytes - invalid args") {
        const uint8_t short_msg[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x80, 0x01};
        const uint8_t inv_protocol[] = {PROTOCOL_VERSION - 1, PROTOCOL_VERSION_INV + 1, 0x80, 0x01};
        const uint8_t inconsistent_protocol[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV + 1, 0x80, 0x01};
        const uint8_t invalid_pl_type[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0xde, 0xad, 0x00, 0x02};
        const uint8_t invalid_pl_length1[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x40, 0x01, 0x00, 0x02, 0x0};
        const uint8_t invalid_pl_length2[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x40, 0x01, 0x00, 0x02, 0x0, 0x0, 0x0};

        CHECK(DoIPMessage::fromRaw(nullptr, 12) == std::nullopt); // null ptr
        CHECK(DoIPMessage::fromRaw(short_msg, sizeof(short_msg)) == std::nullopt); // too short
        CHECK(DoIPMessage::fromRaw(inv_protocol, sizeof(inv_protocol)) == std::nullopt); // wrong protocol
        CHECK(DoIPMessage::fromRaw(inconsistent_protocol, sizeof(inconsistent_protocol)) == std::nullopt); // inconsistent protocol
        CHECK(DoIPMessage::fromRaw(invalid_pl_type, sizeof(invalid_pl_type)) == std::nullopt); // inconsistent payload type
        CHECK(DoIPMessage::fromRaw(invalid_pl_length1, sizeof(invalid_pl_length1)) == std::nullopt); // pl size > payload len
        CHECK(DoIPMessage::fromRaw(invalid_pl_length2, sizeof(invalid_pl_length2)) == std::nullopt); // pl size < payload len
    }

    TEST_CASE("Init from raw bytes - diagnostic message") {
        // Diag message with RDBI request
        const uint8_t example_diag[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x80, 0x01, 0x00, 0x00, 0x00, 0x03, 0x22, 0xFD, 0x10};
        auto opt_msg = DoIPMessage::fromRaw(example_diag, sizeof(example_diag));

        REQUIRE_MESSAGE(opt_msg.has_value(), "No message was created");

        auto msg = opt_msg.value();
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessage);
        CHECK(msg.getPayloadSize() == 3);

        ByteArray msg_conv = msg.toBytes();
        CHECK(msg_conv.size() == 3 + DoIPMessage::DOIP_HEADER_SIZE);

        for(size_t i = 0; i < sizeof(example_diag); i++) {
            CHECK_MESSAGE(msg_conv.at(i) == example_diag[i], "Position ", i, " exp: ", example_diag[i], ", got: ", msg_conv.at(i) );
        }
    }
}
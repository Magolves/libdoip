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
}
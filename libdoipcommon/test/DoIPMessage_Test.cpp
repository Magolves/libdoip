#include <doctest/doctest.h>

#include "DoIPMessage.h"

using namespace doip;

TEST_SUITE("DoIPMessage") {
    TEST_CASE("Message assembly") {
        DoIPMessage msg(DoIPPayloadType::AliveCheckRequest, {0x01, 0x02} );

        CHECK(msg.getPayloadSize() == 2);
        CHECK(msg.getMessageSize() == 10);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckRequest);

        const auto bytes = msg.toBytes();

        CHECK(bytes[0] == 4);
        CHECK(bytes[1] == 0xFB);

    }
}
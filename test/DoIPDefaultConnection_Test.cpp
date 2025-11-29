#include <doctest/doctest.h>

#include "DoIPDefaultConnection.h"
#include "DoIPMessage.h"

namespace doip {

TEST_CASE("DoIPDefaultConnection: Send Protocol Message") {
    auto serverModel = std::make_unique<DoIPServerModel>();
    DoIPDefaultConnection connection(std::move(serverModel));

    DoIPMessage message = message::makeDiagnosticMessage(DoIPAddress(0xCA, 0xFE), DoIPAddress(0xBA, 0xBE), {0xDE, 0xAD, 0xBE, 0xEF});
    CHECK(connection.sendProtocolMessage(message) == 16);
}

TEST_CASE("DoIPDefaultConnection: Close Connection") {
    auto serverModel = std::make_unique<DoIPServerModel>();
    DoIPDefaultConnection connection(std::move(serverModel));

    connection.closeConnection(DoIPCloseReason::ApplicationRequest);
    // No exceptions should be thrown
    CHECK(true);
}

TEST_CASE("DoIPDefaultConnection: Downstream Handler") {
    auto serverModel = std::make_unique<DoIPServerModel>();
    DoIPDefaultConnection connection(std::move(serverModel));

    CHECK_FALSE(connection.hasDownstreamHandler());
}

} // namespace doip
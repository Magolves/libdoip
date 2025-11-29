#include <doctest/doctest.h>

#include "DoIPDefaultConnection.h"
#include "DoIPMessage.h"

namespace doip {

TEST_SUITE("DoIPDefaultConnection") {

    struct DoIPDefaultConnectionTestFixture {
        std::unique_ptr<DoIPDefaultConnection> connection;

        DoIPDefaultConnectionTestFixture()
            : connection(std::make_unique<DoIPDefaultConnection>(std::make_unique<DefaultDoIPServerModel>())) {}
    };

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Get Server Address") {
        DoIPAddress serverAddress = connection->getServerAddress();
        CHECK(serverAddress.hsb() == 0x0E);
        CHECK(serverAddress.lsb() == 0x00);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Send Protocol Message") {
        DoIPMessage message = message::makeDiagnosticMessage(DoIPAddress(0xCA, 0xFE), DoIPAddress(0xBA, 0xBE), {0xDE, 0xAD, 0xBE, 0xEF});
        CHECK(connection->sendProtocolMessage(message) == 16);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Close Connection") {
        CHECK(connection->isOpen() == true);
        CHECK(connection->getCloseReason() == DoIPCloseReason::None);
        connection->closeConnection(DoIPCloseReason::ApplicationRequest);
        CHECK(connection->isOpen() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::ApplicationRequest);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Downstream Handler") {
        CHECK_FALSE(connection->hasDownstreamHandler());
    }
}

} // namespace doip
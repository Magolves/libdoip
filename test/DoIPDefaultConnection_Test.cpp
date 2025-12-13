#include <doctest/doctest.h>

#include "DoIPDefaultConnection.h"
#include "DoIPMessage.h"

namespace doip {

#define WAIT_FOR_STATE(sm, s, max)                                            \
    {                                                                         \
        int counter = 0;                                                      \
        while (sm->getState() != (s) && ++counter < (max)) {                  \
            std::this_thread::sleep_for(10ms);                                \
        };                                                                    \
        std::stringstream ss;                                                 \
        ss << "State " << (s) << " reached after " << ((max) * 10) << "ms\n"; \
        INFO("WAIT_FOR_STATE ", ss.str());                                    \
        CHECK(counter < (max));                                               \
        REQUIRE(sm->getState() == (s));                                       \
    }

TEST_SUITE("DoIPDefaultConnection") {

    struct DoIPDefaultConnectionTestFixture {
        std::unique_ptr<DoIPDefaultConnection> connection;

        DoIPAddress sa = DoIPAddress(0x0E00);

        DoIPDefaultConnectionTestFixture()
            : connection(std::make_unique<DoIPDefaultConnection>(std::make_unique<DefaultDoIPServerModel>())) {

                connection->setAliveCheckTimeout(200ms);
                connection->setGeneralInactivityTimeout(500ms);
                connection->setInitialInactivityTimeout(500ms);
            }
    };

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Get Server Address") {
        DoIPAddress serverAddress = connection->getServerAddress();
        INFO("Server Address: " << serverAddress);
        CHECK(serverAddress == 0x0E00);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Send Protocol Message") {
        DoIPMessage message = message::makeDiagnosticMessage(DoIPAddress(0xCAFE), DoIPAddress(0xBABE), {0xDE, 0xAD, 0xBE, 0xEF});
        CHECK(connection->sendProtocolMessage(message) == 16);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Close Connection basic") {
        doip::Logger::setLevel(spdlog::level::debug);

        CHECK(connection->isOpen() == true);
        CHECK(connection->getState() == DoIPServerState::WaitRoutingActivation);
        CHECK(connection->isRoutingActivated() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::None);
        connection->closeConnection(DoIPCloseReason::ApplicationRequest);
        CHECK(connection->isOpen() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::ApplicationRequest);
        CHECK(connection->getState() == DoIPServerState::Closed);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Invalid message handling") {
        doip::Logger::setLevel(spdlog::level::debug);

        CHECK(connection->isOpen() == true);
        CHECK(connection->getState() == DoIPServerState::WaitRoutingActivation);
        CHECK(connection->getCloseReason() == DoIPCloseReason::None);

        connection->handleMessage2(DoIPMessage()); // Invalid empty message

        CHECK(connection->isOpen() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::InvalidMessage);
        CHECK(connection->getState() == DoIPServerState::Closed);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Timeout after routing activation") {
        doip::Logger::setLevel(spdlog::level::debug);
        CHECK(connection->isOpen() == true);
        CHECK(connection->getState() == DoIPServerState::WaitRoutingActivation);
        CHECK(connection->getCloseReason() == DoIPCloseReason::None);

        connection->handleMessage2(message::makeRoutingActivationRequest(sa));
        CHECK(connection->getState() == DoIPServerState::RoutingActivated);
        CHECK(connection->isRoutingActivated());

        WAIT_FOR_STATE(connection, DoIPServerState::WaitAliveCheckResponse, 100000);
        connection->handleMessage2(message::makeAliveCheckResponse(sa));
        WAIT_FOR_STATE(connection, DoIPServerState::RoutingActivated, 1000);

        WAIT_FOR_STATE(connection, DoIPServerState::WaitAliveCheckResponse, 100000);

        std::this_thread::sleep_for(std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
        std::this_thread::sleep_for(10ms);

        CHECK(connection->isOpen() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::AliveCheckTimeout);
        CHECK(connection->getState() == DoIPServerState::Closed);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Timeout after routing activation bugfix") {
        doip::Logger::setLevel(spdlog::level::debug);

        CHECK(connection->isOpen() == true);
        CHECK(connection->getState() == DoIPServerState::WaitRoutingActivation);
        CHECK(connection->getCloseReason() == DoIPCloseReason::None);

        connection->handleMessage2(message::makeRoutingActivationRequest(DoIPAddress(0xE000)));
        CHECK(connection->getState() == DoIPServerState::RoutingActivated);

        WAIT_FOR_STATE(connection, DoIPServerState::WaitAliveCheckResponse, 100000);

        std::this_thread::sleep_for(std::chrono::milliseconds(times::server::AliveCheckResponseTimeout));
        std::this_thread::sleep_for(10ms);

        CHECK(connection->isOpen() == false);
        CHECK(connection->getCloseReason() == DoIPCloseReason::AliveCheckTimeout);
        CHECK(connection->getState() == DoIPServerState::Closed);
    }

    TEST_CASE_FIXTURE(DoIPDefaultConnectionTestFixture, "DoIPDefaultConnection: Downstream Handler") {
        CHECK_FALSE(connection->hasDownstreamHandler());
    }
}

} // namespace doip
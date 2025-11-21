#include <doctest/doctest.h>
#include <random>

#include "DoIPConnection.h"
#include "DoIPServerStateMachine.h"
#include "Logger.h"

using namespace doip;
using namespace std::chrono_literals;

// Random generator
static std::mt19937 gen(std::random_device{}());
// Extra delay to make sure that timer expired
static auto extraDelay = std::chrono::milliseconds(50);

#define WAIT_FOR_STATE(s, max)                                                \
    {                                                                         \
        int counter = 0;                                                      \
        while (sm.getState() != (s) && ++counter < (max)) {                     \
            std::this_thread::sleep_for(10ms);                                \
        };                                                                    \
        std::stringstream ss;                                                 \
        ss << "State " << (s) << " reached after " << ((max) * 10) << "ms\n"; \
        INFO("WAIT_FOR_STATE ", ss.str());                                    \
        CHECK(counter < (max));                                               \
        REQUIRE(sm.getState() == (s));                                        \
    }

TEST_SUITE("Server state machine") {
    struct ServerStateMachineFixture {
        // use Logger::get()->set_level(spdlog::level::debug); to increase log level

        void handleEvent(DoIPEvent ev, const DoIPMessage *msg) {
            m_log->info("Event {} with msg ", fmt::streamed(ev), fmt::streamed(msg));
            m_lastEvent = ev;
            m_lastMsg = msg;
            m_recvPayloadType = msg->getPayloadType();
        }

        void handleStateChanged(DoIPState from, DoIPState to) {
            INFO("State changed from ", from, " to ", to);
            m_log->info("State changed from {} to {}", fmt::streamed(from), fmt::streamed(to));
            REQUIRE(from != to);
            m_lastFrom = from;
            m_lastTo = to;
        }

        void handleConnectionClosed() {
            m_log->info("Connection closed");
            m_ConnectionClosed = true;
        }

        void handleMessage(const DoIPMessage &msg) {
            m_log->info("Want send {} ", fmt::streamed(msg));
            m_sentPayloadType = msg.getPayloadType();
        }

        DoIPServerStateMachine::CloseSessionHandler onSessionClosed = [this]() { handleConnectionClosed(); };
        DoIPServerStateMachine::StateHandler onEvent = [this](DoIPEvent ev, const DoIPMessage *m) { handleEvent(ev, m); };
        DoIPServerStateMachine::TransitionHandler onStateChanged = [this](DoIPState from, DoIPState to) { handleStateChanged(from, to); };
        DoIPServerStateMachine::SendMessageHandler onMessageSent = [this](const DoIPMessage &msg) { handleMessage(msg); };

        DoIPServerStateMachine sm{};

        DoIPEvent getLastEvent() const { return m_lastEvent; }
        const DoIPMessage *getLastMsg() const { return m_lastMsg; }
        DoIPState getLastFrom() const { return m_lastFrom; }
        DoIPState getLastTo() const { return m_lastTo; }

        DoIPPayloadType getRecvPayloadType() const { return m_recvPayloadType; }
        DoIPPayloadType getSentPayloadType() const { return m_sentPayloadType; }

        bool isConnectionClosed() const { return m_ConnectionClosed; }

        ServerStateMachineFixture() {
            sm.setCloseSessionCallback(onSessionClosed);
            sm.setSendMessageCallback(onMessageSent);
            sm.setTransitionCallback(onStateChanged);
        }

        ~ServerStateMachineFixture() {}

      private:
        DoIPEvent m_lastEvent{};
        const DoIPMessage *m_lastMsg{nullptr};
        DoIPState m_lastFrom{DoIPState::SocketInitialized};
        DoIPState m_lastTo{DoIPState::SocketInitialized};
        bool m_ConnectionClosed{};
        DoIPPayloadType m_recvPayloadType{};
        DoIPPayloadType m_sentPayloadType{};
        std::shared_ptr<spdlog::logger> m_log = Logger::get("test");
    };

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Normal state flow") {
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(getLastEvent() == DoIPEvent::RoutingActivationReceived);
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(getSentPayloadType() == DoIPPayloadType::RoutingActivationResponse);

        sm.processMessage(message::makeDiagnosticMessage(
            DoIPAddress(0xE0, 0x00),
            DoIPAddress(0xE0, 0x01),
            ByteArray{0x03, 0x22, 0xFD, 0x11}));
        REQUIRE(getSentPayloadType() == DoIPPayloadType::DiagnosticMessageAck);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Timeout after init") {
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        std::this_thread::sleep_for(times::server::InitialInactivityTimeout);
        std::this_thread::sleep_for(extraDelay);

        REQUIRE(sm.getState() == DoIPState::Closed);
        REQUIRE(isConnectionClosed());
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check with response") {
        Logger::get()->set_level(spdlog::level::debug); // to increase log level
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(getLastEvent() == DoIPEvent::RoutingActivationReceived);
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(getSentPayloadType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        WAIT_FOR_STATE(DoIPState::WaitAliveCheckResponse, 300);

        REQUIRE(sm.getState() == DoIPState::WaitAliveCheckResponse);
        REQUIRE(getSentPayloadType() == DoIPPayloadType::AliveCheckRequest);

        // Send check alive response
        sm.processMessage(message::makeAliveCheckResponse(
            DoIPAddress(0xE0, 0x00)));
        // Server should be fine again
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check without response") {
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(getLastEvent() == DoIPEvent::RoutingActivationReceived);
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(getSentPayloadType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);

        WAIT_FOR_STATE(DoIPState::Closed, 300);
    }
}

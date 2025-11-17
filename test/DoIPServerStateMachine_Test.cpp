#include <doctest/doctest.h>
#include "DoIPServerStateMachine.h"
#include "DoIPConnection.h"
#include "Logger.h"

using namespace doip;

TEST_SUITE("Server state machine") {
    struct ServerStateMachineFixture {



        void handleEvent(DoIPEvent ev, const DoIPMessage *msg) {
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
            m_sentPayloadType = msg.getPayloadType();
        }

        DoIPServerStateMachine::CloseConnectionHandler onConnectionClosed = [this](){handleConnectionClosed();};
        DoIPServerStateMachine::StateHandler onEvent = [this](DoIPEvent ev, const DoIPMessage *m) {handleEvent(ev, m);};
        DoIPServerStateMachine::TransitionHandler onStateChanged = [this](DoIPState from, DoIPState to) {handleStateChanged(from, to);};
        DoIPServerStateMachine::SendMessageCallback onMessageSent = [this](const DoIPMessage &msg) {handleMessage(msg);};

        DoIPServerStateMachine sm{onConnectionClosed};


        DoIPEvent getLastEvent() const { return m_lastEvent; }
        const DoIPMessage* getLastMsg() const { return m_lastMsg; }
        DoIPState getLastFrom() const { return m_lastFrom; }
        DoIPState getLastTo() const { return m_lastTo; }

        DoIPPayloadType getRecvPayloadType() const {return m_recvPayloadType;}
        DoIPPayloadType getSentPayloadType() const {return m_sentPayloadType;}

        bool isConnectionClosed() const { return m_ConnectionClosed; }

        ServerStateMachineFixture() {
            sm.setSendMessageCallback(onMessageSent);
            sm.setTransitionCallback(onStateChanged);
        }

        ~ServerStateMachineFixture() {}


        private:
            DoIPEvent m_lastEvent{};
            const DoIPMessage *m_lastMsg;
            DoIPState m_lastFrom{DoIPState::SocketInitialized};
            DoIPState m_lastTo{DoIPState::SocketInitialized};
            bool m_ConnectionClosed{};
            DoIPPayloadType m_recvPayloadType;
            DoIPPayloadType m_sentPayloadType;
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
            ByteArray{0x03, 0x22, 0xFD, 0x11 }
        ));
        REQUIRE(getSentPayloadType() == DoIPPayloadType::DiagnosticMessageAck);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Timeout after init") {
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        std::this_thread::sleep_for(std::chrono::milliseconds(times::server::InitialInactivityTimeout + 10));

        REQUIRE(sm.getState() == DoIPState::Closed);
        REQUIRE(isConnectionClosed());
    }
}

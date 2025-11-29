#include <doctest/doctest.h>
#include <random>

#include "DoIPConnection.h"
#include "DoIPServerStateMachine.h"
#include "DoIPDefaultConnection.h"
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



// Mock implementation of IConnectionContext for testing
class MockConnectionContext : public DoIPDefaultConnection {
public:
    // Captured data
    std::vector<DoIPMessage> sentMessages;
    DoIPCloseReason closeReason = DoIPCloseReason::None;
    DoIPAddress serverAddress{0x0E, 0x80};
    DoIPAddress clientAddress = DoIPAddress::ZeroAddress;
    bool connectionClosed = false;
    DoIPDiagnosticAck diagnosticResponse = std::nullopt;

    std::shared_ptr<spdlog::logger> logger = Logger::get("mock");

    // Callbacks for test verification
    std::function<void(DoIPServerState, DoIPServerState)> transitionCallback;

    MockConnectionContext()
        : DoIPDefaultConnection(std::make_unique<DefaultDoIPServerModel>()) {}

    // IConnectionContext implementation
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override {
        sentMessages.push_back(msg);
        logger->info("Sent message: {}", fmt::streamed(msg));
        return static_cast<ssize_t>(msg.size());
    }

    void closeConnection(DoIPCloseReason reason) override {
        closeReason = reason;
        connectionClosed = true;
    }

    DoIPAddress getServerAddress() const override {
        return serverAddress;
    }

    DoIPAddress getClientAddress() const override {
        return clientAddress;
    }

    void setClientAddress(const DoIPAddress& address) override {
        clientAddress = address;
    }

    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override {
        (void)msg;
        return diagnosticResponse;
    }

    void notifyConnectionClosed(DoIPCloseReason reason) override {
        logger->info("Connection closed, reason: {}", fmt::streamed(reason));
    }

    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override {
        (void)ack;
    }

    // Test helpers
    void clearSentMessages() {
        sentMessages.clear();
    }

    DoIPPayloadType getLastSentMessageType() const {
        if (sentMessages.empty()) {
            return DoIPPayloadType::NegativeAck;
        }
        return sentMessages.back().getPayloadType();
    }

};

TEST_SUITE("Server state machine") {
    struct ServerStateMachineFixture {
        MockConnectionContext mockContext;
        DoIPServerStateMachine sm{mockContext};
        std::shared_ptr<spdlog::logger> m_log = Logger::get("test");

        void handleStateChanged(DoIPServerState from, DoIPServerState to) {
            INFO("State changed from ", from, " to ", to);
            m_log->info("State changed from {} to {}", fmt::streamed(from), fmt::streamed(to));
            REQUIRE(from != to);
        }

        ServerStateMachineFixture() {
            sm.setTransitionCallback([this](DoIPServerState from, DoIPServerState to) {
                handleStateChanged(from, to);
            });
        }

        ~ServerStateMachineFixture() {}
    };

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Normal state flow bugfix1") {
        REQUIRE(sm.getState() == DoIPServerState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        sm.processMessage(message::makeDiagnosticMessage(
            DoIPAddress(0xE0, 0x00),
            DoIPAddress(0xE0, 0x01),
            ByteArray{0x03, 0x22, 0xFD, 0x11}));
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::DiagnosticMessageAck);

        if (mockContext.hasDownstreamHandler()) {
            WAIT_FOR_STATE(DoIPServerState::WaitDownstreamResponse, 300);
            REQUIRE(sm.getState() == DoIPServerState::WaitDownstreamResponse);

            // Simulate downstream response received
            mockContext.receiveDownstreamResponse(
                message::makeDiagnosticMessage(
                    DoIPAddress(0xE0, 0x00),
                    DoIPAddress(0xE0, 0x01),
                    ByteArray{0x03, 0x62, 0xFD, 0x11, 0x66}));
        }
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Timeout after init") {
        REQUIRE(sm.getState() == DoIPServerState::WaitRoutingActivation);
        std::this_thread::sleep_for(times::server::InitialInactivityTimeout);
        std::this_thread::sleep_for(extraDelay);

        REQUIRE(sm.getState() == DoIPServerState::Closed);
        REQUIRE(mockContext.connectionClosed);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check with response") {
        Logger::get()->set_level(spdlog::level::debug); // to increase log level
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPServerState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        WAIT_FOR_STATE(DoIPServerState::WaitAliveCheckResponse, 300);

        REQUIRE(sm.getState() == DoIPServerState::WaitAliveCheckResponse);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::AliveCheckRequest);

        // Send check alive response
        sm.processMessage(message::makeAliveCheckResponse(
            DoIPAddress(0xE0, 0x00)));
        // Server should be fine again
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check with diagnostic message response") {
        Logger::get()->set_level(spdlog::level::debug); // to increase log level
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPServerState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        WAIT_FOR_STATE(DoIPServerState::WaitAliveCheckResponse, 300);

        REQUIRE(sm.getState() == DoIPServerState::WaitAliveCheckResponse);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::AliveCheckRequest);

        // Send diagnostic message instead of alive check response
        sm.processMessage(message::makeDiagnosticMessage(
            DoIPAddress(0xE0, 0x00), DoIPAddress(0xE0, 0x01), ByteArray{0x03, 0x22, 0xFD, 0x11}));
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::DiagnosticMessageAck);
        // Server should be fine again
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check without response") {
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPServerState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPServerState::RoutingActivated);

        WAIT_FOR_STATE(DoIPServerState::Closed, 300);
    }
}

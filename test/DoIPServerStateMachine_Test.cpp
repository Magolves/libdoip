#include <doctest/doctest.h>
#include <random>

#include "DoIPConnection.h"
#include "DoIPServerStateMachine.h"
#include "IConnectionContext.h"
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
class MockConnectionContext : public IConnectionContext {
public:
    // Captured data
    std::vector<DoIPMessage> sentMessages;
    CloseReason closeReason = CloseReason::None;
    DoIPAddress serverAddress{0x0E, 0x80};
    uint16_t activeSourceAddress = 0;
    bool connectionClosed = false;
    DoIPDiagnosticAck diagnosticResponse = std::nullopt;

    // Callbacks for test verification
    std::function<void(DoIPState, DoIPState)> transitionCallback;

    // IConnectionContext implementation
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override {
        sentMessages.push_back(msg);
        return static_cast<ssize_t>(msg.size());
    }

    void closeConnection(CloseReason reason) override {
        closeReason = reason;
        connectionClosed = true;
    }

    DoIPAddress getServerAddress() const override {
        return serverAddress;
    }

    uint16_t getActiveSourceAddress() const override {
        return activeSourceAddress;
    }

    void setActiveSourceAddress(uint16_t address) override {
        activeSourceAddress = address;
    }

    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override {
        (void)msg;
        return diagnosticResponse;
    }

    void notifyConnectionClosed(CloseReason reason) override {
        (void)reason;
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

        void handleStateChanged(DoIPState from, DoIPState to) {
            INFO("State changed from ", from, " to ", to);
            m_log->info("State changed from {} to {}", fmt::streamed(from), fmt::streamed(to));
            REQUIRE(from != to);
        }

        ServerStateMachineFixture() {
            sm.setTransitionCallback([this](DoIPState from, DoIPState to) {
                handleStateChanged(from, to);
            });
        }

        ~ServerStateMachineFixture() {}
    };

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Normal state flow") {
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        sm.processMessage(message::makeDiagnosticMessage(
            DoIPAddress(0xE0, 0x00),
            DoIPAddress(0xE0, 0x01),
            ByteArray{0x03, 0x22, 0xFD, 0x11}));
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::DiagnosticMessageAck);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Timeout after init") {
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        std::this_thread::sleep_for(times::server::InitialInactivityTimeout);
        std::this_thread::sleep_for(extraDelay);

        REQUIRE(sm.getState() == DoIPState::Closed);
        REQUIRE(mockContext.connectionClosed);
    }

    TEST_CASE_FIXTURE(ServerStateMachineFixture, "Alive check with response") {
        Logger::get()->set_level(spdlog::level::debug); // to increase log level
        auto generalInactivityTimeout = std::chrono::milliseconds(std::uniform_int_distribution<>(500, 2000)(gen));
        sm.setGeneralInactivityTimeout(generalInactivityTimeout);

        // Activate routing
        REQUIRE(sm.getState() == DoIPState::WaitRoutingActivation);
        sm.processMessage(message::makeRoutingActivationRequest(DoIPAddress(0xE0, 0x00)));
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        WAIT_FOR_STATE(DoIPState::WaitAliveCheckResponse, 300);

        REQUIRE(sm.getState() == DoIPState::WaitAliveCheckResponse);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::AliveCheckRequest);

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
        REQUIRE(sm.getState() == DoIPState::RoutingActivated);
        REQUIRE(mockContext.getLastSentMessageType() == DoIPPayloadType::RoutingActivationResponse);

        REQUIRE(sm.getState() == DoIPState::RoutingActivated);

        WAIT_FOR_STATE(DoIPState::Closed, 300);
    }
}

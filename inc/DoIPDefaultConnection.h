#ifndef DOIPDEFAULTCONNECTION_H
#define DOIPDEFAULTCONNECTION_H

#include "IConnectionContext.h"
#include "DoIPServerStateMachine.h"
#include "DoIPServerModel.h"
#include <optional>

namespace doip {

/**
 * @brief Default implementation of IConnectionContext
 *
 * This class provides a default implementation of the IConnectionContext
 * interface, including the state machine and server model. It excludes
 * TCP socket support, making it suitable for non-TCP-based connections.
 */
class DoIPDefaultConnection : public IConnectionContext {
public:
    explicit DoIPDefaultConnection(UniqueServerModelPtr model);

    // === IConnectionContext interface implementation ===
    ssize_t sendProtocolMessage(const DoIPMessage &msg) override;
    void closeConnection(DoIPCloseReason reason) override;

    bool isOpen() const override {
        return m_isOpen;
    }

    DoIPCloseReason getCloseReason() const override {
        return m_closeReason;
    }

    DoIPAddress getServerAddress() const override;
    DoIPAddress getClientAddress() const override;
    void setClientAddress(const DoIPAddress& address) override;
    DoIPDiagnosticAck notifyDiagnosticMessage(const DoIPMessage &msg) override;
    void notifyConnectionClosed(DoIPCloseReason reason) override;
    void notifyDiagnosticAckSent(DoIPDiagnosticAck ack) override;
    bool hasDownstreamHandler() const override;
    DoIPDownstreamResult notifyDownstreamRequest(const DoIPMessage &msg) override;
    void receiveDownstreamResponse(const DoIPMessage &response) override;
    void notifyDownstreamResponseReceived(const DoIPMessage &request, const DoIPMessage &response) override;

    DoIPServerState getState() const {
        return m_stateMachine.getState();
    }

    UniqueServerModelPtr& getServerModel() {
        return m_serverModel;
    }

protected:
    DoIPServerStateMachine m_stateMachine;
    UniqueServerModelPtr m_serverModel;
    DoIPAddress m_routedClientAddress;
    bool m_isOpen;
    DoIPCloseReason m_closeReason = DoIPCloseReason::None;

};

} // namespace doip

#endif /* DOIPDEFAULTCONNECTION_H */
#pragma once
#include <chrono>
#include <memory>
#include "IDownstreamProvider.h"
#include "DoIPMessage.h"
#include "DoIPServerModel.h"

class DownstreamForwarder {
public:
    explicit DownstreamForwarder(std::shared_ptr<IDownstreamProvider> provider)
        : m_provider(std::move(provider)) {}

    /**
     * Forward a DoIP message downstream, process the provider response,
     * and call the DoIP-level downstream callback.
     */
    void forward(
        const DoIPMessage& msg,
        ServerModelDownstreamResponseHandler doipCallback)
    {
        if (!m_provider) {
            DownstreamResponse resp;
            resp.status = DownstreamStatus::Error;
            return doipCallback(resp.payload, DoIPDownstreamResult::Error);
        }

        auto [data, size] = msg.getDiagnosticMessagePayload();
        ByteArray request(data, data + size);
        auto start = std::chrono::steady_clock::now();

        m_provider->sendRequest(request,
            [start, doipCallback](const DownstreamResponse& resp){
                // Map provider response to DoIP response type:
                DoIPDownstreamResult dr = DoIPDownstreamResult::Error;
                switch(resp.status) {
                    case DownstreamStatus::Handled: dr = DoIPDownstreamResult::Handled; break;
                    case DownstreamStatus::Pending: dr = DoIPDownstreamResult::Pending; break;
                    case DownstreamStatus::Timeout: dr = DoIPDownstreamResult::Timeout; break;
                    default: break;
                }

                // Hand response back to DoIP stack
                doipCallback(resp.payload, dr);
            }
        );
    }

private:
    std::shared_ptr<IDownstreamProvider> m_provider;
};

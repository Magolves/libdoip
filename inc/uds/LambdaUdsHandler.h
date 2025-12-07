#ifndef LAMBDAUDSHANDLER_H
#define LAMBDAUDSHANDLER_H

#include "IUdsServiceHandler.h"
#include <functional>

namespace doip::uds {

class LambdaUdsHandler : public IUdsServiceHandler {
public:
    using Fn = std::function<UdsResponse(const ByteArray &)>;
    explicit LambdaUdsHandler(Fn fn) : m_fn(std::move(fn)) {}
    UdsResponse handle(const ByteArray &request) override { return m_fn(request); }

private:
    Fn m_fn;
};

} // namespace doip::uds

#endif // LAMBDAUDSHANDLER_H

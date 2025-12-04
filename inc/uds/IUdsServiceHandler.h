#ifndef IUDSSERVICEHANDLER_H
#define IUDSSERVICEHANDLER_H

#include "UdsResponseCode.h"
#include "DoIPMessage.h"
#include <memory>

namespace doip::uds {

using UdsResponse = std::pair<UdsResponseCode, ByteArray>;

class IUdsServiceHandler {
public:
    virtual ~IUdsServiceHandler() = default;
    virtual UdsResponse handle(const ByteArray &request) = 0;
};

using IUdsServiceHandlerPtr = std::unique_ptr<IUdsServiceHandler>;

} // namespace doip::uds

#endif // IUDSSERVICEHANDLER_H

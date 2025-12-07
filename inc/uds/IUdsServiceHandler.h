#ifndef IUDSSERVICEHANDLER_H
#define IUDSSERVICEHANDLER_H

#include "UdsResponseCode.h"
#include "DoIPMessage.h"
#include <memory>

namespace doip::uds {

using UdsResponse = std::pair<UdsResponseCode, ByteArray>;

inline std::ostream &operator<<(std::ostream &os, const UdsResponse &response) {
    std::ios_base::fmtflags flags(os.flags());

    os << response.first << " [";
    os << std::hex << std::uppercase << std::setw(2) << std::setfill('0');

    for (size_t i = 0; i < response.second.size(); ++i) {
        if (i > 0) {
            os << '.';
        }
        os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(response.second[i]);
    }

    os.flags(flags);
    return os;
}

class IUdsServiceHandler {
public:
    virtual ~IUdsServiceHandler() = default;
    virtual UdsResponse handle(const ByteArray &request) = 0;
};

using IUdsServiceHandlerPtr = std::unique_ptr<IUdsServiceHandler>;

} // namespace doip::uds

#endif // IUDSSERVICEHANDLER_H

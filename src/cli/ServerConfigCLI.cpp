#include "cli/ServerConfigCLI.h"

#include <stdexcept>
#include <cctype>

namespace doip::cli {

ServerConfigCLI::ServerConfigCLI() : m_app{"DoIP Server"} {
    m_app.add_option("--vin", m_vinStr, "VIN (17 ASCII chars)")
        ->check(CLI::Validator(
            [](const std::string &s) {
                return s.size() == 17 ? std::string() : std::string{"VIN must be 17 chars"};
            },
            "vinlen"));

    m_app.add_option("--eid", m_eidHex, "EID (12 hex chars, e.g., 112233445566)")
        ->check(CLI::Validator(
            [](const std::string &s) {
                return s.size() == 12 ? std::string() : std::string{"EID must be 12 hex chars"};
            },
            "eidlen"));

    m_app.add_option("--gid", m_gidHex, "GID (12 hex chars)")
        ->check(CLI::Validator(
            [](const std::string &s) {
                return s.size() == 12 ? std::string() : std::string{"GID must be 12 hex chars"};
            },
            "gidlen"));

    m_app.add_option("--logical-address", m_logicalStr, "Logical address (hex or dec, default 0x28)");
    m_app.add_flag("--loopback", m_loopback, "Use loopback announcements (127.0.0.1)");
    m_app.add_flag("--daemonize", m_daemonize, "Run as daemon");
    m_app.add_option("--announce-count", m_announceCount, "Announcement count");
    m_app.add_option("--announce-interval", m_announceInterval, "Announcement interval (ms)");
}

CLI::App &ServerConfigCLI::app() { return m_app; }

bool ServerConfigCLI::parseHexBytes12(const std::string &s, std::array<uint8_t, 6> &out) {
    if (s.size() != 12) return false;
    auto isHex = std::all_of(s.begin(), s.end(), [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c));
    });

    if (!isHex) return false;

    for (size_t i = 0; i < 6; ++i) {
        auto byte = std::stoul(s.substr(i * 2, 2), nullptr, 16);
        out[i] = static_cast<uint8_t>(byte & 0xFF);
    }
    return true;
}

ServerConfig ServerConfigCLI::parse_and_build(int argc, char **argv) {
    try {
        m_app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // Print error/help via CLI11 and signal failure to caller
        m_app.exit(e);
        throw std::runtime_error("CLI parse error");
    }

    ServerConfig cfg;
    cfg.loopback = m_loopback;
    cfg.daemonize = m_daemonize;
    cfg.announceCount = m_announceCount;
    cfg.announceInterval = m_announceInterval;

    if (!m_vinStr.empty()) {
        cfg.vin = DoIpVin(m_vinStr);
    }

    if (!m_eidHex.empty()) {
        std::array<uint8_t, 6> b{};
        if (!parseHexBytes12(m_eidHex, b)) {
            throw std::runtime_error("Invalid EID: must be 12 hex chars");
        }
        cfg.eid = DoIpEid(b.data(), 6);
    }

    if (!m_gidHex.empty()) {
        std::array<uint8_t, 6> b{};
        if (!parseHexBytes12(m_gidHex, b)) {
            throw std::runtime_error("Invalid GID: must be 12 hex chars");
        }
        cfg.gid = DoIpGid(b.data(), 6);
    }

    unsigned long la = 0;
    // Strict validation: hex with 0x prefix or decimal; ensure full string parses
    if (m_logicalStr.size() > 2 && m_logicalStr[0] == '0' && (m_logicalStr[1] == 'x' || m_logicalStr[1] == 'X')) {
        const std::string hex = m_logicalStr.substr(2);
        if (hex.empty()) {
            throw std::runtime_error("Invalid logical-address: empty hex after 0x");
        }
        auto isHex = std::all_of(hex.begin(), hex.end(), [](char c) {
            return std::isxdigit(static_cast<unsigned char>(c));
        });
        if (!isHex) {
            throw std::runtime_error("Invalid logical-address: non-hex digit present");
        }
        char *end = nullptr;
        la = std::strtoul(hex.c_str(), &end, 16);
        if (end == nullptr || *end != '\0') {
            throw std::runtime_error("Invalid logical-address: partial parse");
        }
    } else {
        char *end = nullptr;
        la = std::strtoul(m_logicalStr.c_str(), &end, 10);
        if (end == nullptr || *end != '\0') {
            throw std::runtime_error("Invalid logical-address: partial parse");
        }
    }
    cfg.logicalAddress = DoIPAddress(la & 0xFFFF);

    return cfg;
}

} // namespace doip::cli
#ifndef SERVERCONFIGCLI_H
#define SERVERCONFIGCLI_H

#include <CLI/CLI.hpp>
#include <string>
#include <array>

#include "DoIPServer.h"
#include "DoIPIdentifiers.h"

namespace doip::cli {

/**
 * Build a CLI11 app configured with options to populate ServerConfig.
 * Call parse_and_build to parse argc/argv and get a filled ServerConfig.
 */
class ServerConfigCLI {
  public:
    ServerConfigCLI();

    /** Return the configured CLI11 app for further customization if needed */
    CLI::App &app();

    /** Parse argv and return a populated ServerConfig (throws on parse errors) */
    ServerConfig parse_and_build(int argc, char **argv);

  private:
    CLI::App m_app;

    // Raw option storage
    std::string m_vinStr;
    std::string m_eidHex;
    std::string m_gidHex;
    std::string m_logicalStr{"0x28"};
    bool m_loopback{false};
    bool m_daemonize{false};
    int m_announceCount{3};
    unsigned m_announceInterval{500};

    static bool parseHexBytes12(const std::string &s, std::array<uint8_t, 6> &out);
};

} // namespace doip::cli

#endif // SERVERCONFIGCLI_H

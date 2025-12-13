#include <doctest/doctest.h>
#include "cli/ServerConfigCLI.h"

using namespace doip;
using namespace doip::cli;

TEST_SUITE("ServerConfigCLI") {

TEST_CASE("parses valid arguments into ServerConfig") {
    ServerConfigCLI cli;
    const char *argv[] = {
        "prog",
        "--vin", "12345678901234567",
        "--eid", "112233445566",
        "--gid", "aabbccddeeff",
        "--logical-address", "0x28",
        "--announce-count", "5",
        "--announce-interval", "250",
        "--loopback"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);

    auto cfg = cli.parse_and_build(argc, const_cast<char **>(argv));
    CHECK(cfg.loopback == true);
    CHECK(cfg.announceCount == 5);
    CHECK(cfg.announceInterval == 250);
    CHECK(static_cast<uint16_t>(cfg.logicalAddress) == 0x28);
}

TEST_CASE("invalid EID raises error") {
    ServerConfigCLI cli;
    const char *argv[] = { "prog", "--eid", "123" };
    int argc = sizeof(argv) / sizeof(argv[0]);
    CHECK_THROWS(cli.parse_and_build(argc, const_cast<char **>(argv)));
}

TEST_CASE("invalid GID raises error") {
    ServerConfigCLI cli;
    const char *argv[] = { "prog", "--gid", "xyzxyzxyzxyz" };
    int argc = sizeof(argv) / sizeof(argv[0]);
    CHECK_THROWS(cli.parse_and_build(argc, const_cast<char **>(argv)));
}

TEST_CASE("invalid logical address raises error") {
    ServerConfigCLI cli;
    const char *argv[] = { "prog", "--logical-address", "0xzz" };
    int argc = sizeof(argv) / sizeof(argv[0]);
    CHECK_THROWS(cli.parse_and_build(argc, const_cast<char **>(argv)));
}

}
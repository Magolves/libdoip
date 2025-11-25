#include <doctest/doctest.h>
#include "MacAddress.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sstream>

using namespace doip;

TEST_SUITE("MacAddress") {

    TEST_CASE("getMacAddress with nullptr returns first interface") {
        MacAddress mac;

        // This should attempt to get the first available interface
        bool result = getMacAddress(nullptr, mac);

        // On systems with network interfaces, this should succeed
        // We can't guarantee it will succeed in all test environments
        if (result) {
            INFO("Successfully retrieved MAC address from first interface");

            // Check that it's not all zeros
            bool allZeros = true;
            for (size_t i = 0; i < 6; ++i) {
                if (mac[i] != 0) {
                    allZeros = false;
                    break;
                }
            }

            CHECK_FALSE(allZeros);
        } else {
            WARN("Could not retrieve MAC address from first interface (may be expected in some environments)");
        }
    }

    TEST_CASE("getFirstMacAddress") {
        MacAddress mac;

        bool result = getFirstMacAddress(mac);

        if (result) {
            INFO("Successfully retrieved first MAC address");

            // Check that it's not all zeros
            bool allZeros = true;
            for (size_t i = 0; i < 6; ++i) {
                if (mac[i] != 0) {
                    allZeros = false;
                    break;
                }
            }

            CHECK_FALSE(allZeros);

            // Print MAC address for debugging (won't show in successful tests)
            std::ostringstream oss;
            oss << std::hex << std::uppercase << std::setfill('0');
            for (size_t i = 0; i < 6; ++i) {
                if (i > 0) oss << ":";
                oss << std::setw(2) << static_cast<unsigned int>(mac[i]);
            }
            MESSAGE("First MAC address: " << oss.str());
        } else {
            WARN("Could not retrieve first MAC address");
        }
    }

    TEST_CASE("getMacAddress with specific interface") {
        MacAddress mac;

        // Try different common interface names based on platform
#if defined(__linux__)
        const char* interfaces[] = {"eth0", "ens33", "enp0s3", "wlan0", "wlp2s0"};
#elif defined(__APPLE__)
        const char* interfaces[] = {"en0", "en1", "en2"};
#elif defined(_WIN32) || defined(_WIN64)
        const char* interfaces[] = {"Ethernet", "Wi-Fi", "Local Area Connection"};
#else
        const char* interfaces[] = {"eth0"};
#endif

        bool foundAny = false;
        std::string successfulInterface;

        for (const char* iface : interfaces) {
            if (getMacAddress(iface, mac)) {
                foundAny = true;
                successfulInterface = iface;

                // Check that it's not all zeros
                bool allZeros = true;
                for (size_t i = 0; i < 6; ++i) {
                    if (mac[i] != 0) {
                        allZeros = false;
                        break;
                    }
                }

                if (!allZeros) {
                    INFO("Found interface: " << iface);
                    break;
                }
            }
        }

        if (foundAny) {
            MESSAGE("Successfully retrieved MAC from interface: " << successfulInterface);
        } else {
            WARN("Could not find any of the common interface names");
        }
    }

    TEST_CASE("getMacAddress with invalid interface name") {
        MacAddress mac;

        // Try to get MAC address from a non-existent interface
        bool result = getMacAddress("thisinterfacedoesnotexist999", mac);

        // This should fail
        CHECK_FALSE(result);
    }

    TEST_CASE("MAC address format validation") {
        MacAddress mac;

        if (getFirstMacAddress(mac)) {
            // Check that at least one byte is non-zero
            // (all-zero MAC addresses are typically invalid)
            bool hasNonZero = false;
            for (size_t i = 0; i < 6; ++i) {
                if (mac.at(i) != 0) {
                    hasNonZero = true;
                    break;
                }
            }

            CHECK(hasNonZero);
        }
    }

    TEST_CASE("MAC address consistency") {
        MacAddress mac1;
        MacAddress mac2;

        if (getFirstMacAddress(mac1) && getFirstMacAddress(mac2)) {
            // Getting the first MAC address twice should return the same result
            CHECK(mac1 == mac2);
        }
    }

    TEST_CASE("MAC address array operations") {
        MacAddress mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

        CHECK(mac[0] == 0x00);
        CHECK(mac[1] == 0x11);
        CHECK(mac[2] == 0x22);
        CHECK(mac[3] == 0x33);
        CHECK(mac[4] == 0x44);
        CHECK(mac[5] == 0x55);

        // Test array access
        CHECK(mac.data() != nullptr);
    }

    TEST_CASE("Platform detection (informational)") {
#if defined(_WIN32) || defined(_WIN64)
        MESSAGE("Platform: Windows");
#elif defined(__APPLE__) && defined(__MACH__)
        MESSAGE("Platform: macOS");
#elif defined(__linux__)
        MESSAGE("Platform: Linux");
#else
        MESSAGE("Platform: Unknown");
#endif
    }
}

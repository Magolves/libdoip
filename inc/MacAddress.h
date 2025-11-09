#ifndef MACADDRESS_H
#define MACADDRESS_H

#include <array>
#include <cstdint>

namespace doip {

/**
 * @brief Type alias for MAC address (6 bytes)
 */
using MacAddress = std::array<uint8_t, 6>;

/**
 * @brief Retrieves the MAC address of a network interface.
 *
 * This function is cross-platform and works on Linux, macOS, and Windows.
 * It retrieves the hardware (MAC) address of the specified network interface.
 *
 * @param ifname The name of the network interface (e.g., "eth0" on Linux, "en0" on macOS, "Ethernet" on Windows)
 *               If nullptr, the function will attempt to find the first available Ethernet interface.
 * @param mac Reference to a MacAddress where the result will be stored
 * @return true if the MAC address was successfully retrieved, false otherwise
 *
 * @note On Linux: Requires interface names like "eth0", "ens33", "wlan0", etc.
 * @note On macOS: Requires interface names like "en0", "en1", etc.
 * @note On Windows: Requires adapter names like "Ethernet", "Wi-Fi", etc.
 *
 * @example
 * MacAddress mac;
 * if (getMacAddress("eth0", mac)) {
 *     // MAC address successfully retrieved
 *     printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
 *            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
 * }
 */
bool getMacAddress(const char* ifname, MacAddress& mac);

/**
 * @brief Retrieves the MAC address of the first available network interface.
 *
 * This is a convenience function that attempts to find and return the MAC address
 * of the first available network interface on the system.
 *
 * @param mac Reference to a MacAddress where the result will be stored
 * @return true if a MAC address was successfully retrieved, false otherwise
 */
bool getFirstMacAddress(MacAddress& mac);

} // namespace doip

#endif /* MACADDRESS_H */

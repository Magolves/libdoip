#include "MacAddress.h"
#include <cstring>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #include <sys/socket.h>
    #include <sys/sysctl.h>
    #include <net/if.h>
    #include <net/if_dl.h>
    #include <ifaddrs.h>
    #include <arpa/inet.h>
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <net/if.h>
    #include <unistd.h>
    #include <cstring>
    #include <ifaddrs.h>
#else
    #error "Unsupported platform"
#endif

namespace doip {

#ifdef PLATFORM_LINUX

/**
 * @brief Linux implementation using ioctl with SIOCGIFHWADDR
 */
bool getMacAddress(const char* ifname, MacAddress& mac) {
    if (ifname == nullptr) {
        return getFirstMacAddress(mac);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return false;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        close(sock);
        return false;
    }

    close(sock);

    // Copy MAC address from ifr_hwaddr.sa_data
    std::memcpy(mac.data(), ifr.ifr_hwaddr.sa_data, 6);

    return true;
}

bool getFirstMacAddress(MacAddress& mac) {
    struct ifaddrs* ifaddr = nullptr;
    struct ifaddrs* ifa = nullptr;

    if (getifaddrs(&ifaddr) == -1) {
        return false;
    }

    bool found = false;

    // Iterate through all interfaces
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        // Skip loopback interface
        if (std::strcmp(ifa->ifa_name, "lo") == 0) {
            continue;
        }

        // Try to get MAC address for this interface
        if (getMacAddress(ifa->ifa_name, mac)) {
            // Check if it's not all zeros
            bool isZero = true;
            for (size_t i = 0; i < 6; ++i) {
                if (mac[i] != 0) {
                    isZero = false;
                    break;
                }
            }

            if (!isZero) {
                found = true;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return found;
}

#elif defined(PLATFORM_MACOS)

/**
 * @brief macOS implementation using getifaddrs and AF_LINK
 */
bool getMacAddress(const char* ifname, MacAddress& mac) {
    if (ifname == nullptr) {
        return getFirstMacAddress(mac);
    }

    struct ifaddrs* ifaddr = nullptr;
    struct ifaddrs* ifa = nullptr;

    if (getifaddrs(&ifaddr) == -1) {
        return false;
    }

    bool found = false;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        // Check if this is the interface we're looking for
        if (std::strcmp(ifa->ifa_name, ifname) != 0) {
            continue;
        }

        // Check if this is a link-layer address (AF_LINK for macOS)
        if (ifa->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);

            if (sdl->sdl_alen == 6) {
                unsigned char* ptr = reinterpret_cast<unsigned char*>(LLADDR(sdl));
                std::memcpy(mac.data(), ptr, 6);
                found = true;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return found;
}

bool getFirstMacAddress(MacAddress& mac) {
    struct ifaddrs* ifaddr = nullptr;
    struct ifaddrs* ifa = nullptr;

    if (getifaddrs(&ifaddr) == -1) {
        return false;
    }

    bool found = false;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        // Skip loopback interface
        if (std::strcmp(ifa->ifa_name, "lo0") == 0) {
            continue;
        }

        // Check if this is a link-layer address
        if (ifa->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);

            if (sdl->sdl_alen == 6) {
                unsigned char* ptr = reinterpret_cast<unsigned char*>(LLADDR(sdl));

                // Check if it's not all zeros
                bool isZero = true;
                for (int i = 0; i < 6; ++i) {
                    if (ptr[i] != 0) {
                        isZero = false;
                        break;
                    }
                }

                if (!isZero) {
                    std::memcpy(mac.data(), ptr, 6);
                    found = true;
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
    return found;
}

#elif defined(PLATFORM_WINDOWS)

/**
 * @brief Windows implementation using IP Helper API
 */
bool getMacAddress(const char* ifname, MacAddress& mac) {
    if (ifname == nullptr) {
        return getFirstMacAddress(mac);
    }

    ULONG bufferSize = 15000;
    PIP_ADAPTER_INFO adapterInfo = nullptr;
    PIP_ADAPTER_INFO adapter = nullptr;

    adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new BYTE[bufferSize]);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        delete[] reinterpret_cast<BYTE*>(adapterInfo);
        adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new BYTE[bufferSize]);
    }

    bool found = false;

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        adapter = adapterInfo;

        while (adapter) {
            // Compare adapter name or description
            if (std::strcmp(adapter->AdapterName, ifname) == 0 ||
                std::strcmp(adapter->Description, ifname) == 0) {

                if (adapter->AddressLength == 6) {
                    std::memcpy(mac.data(), adapter->Address, 6);
                    found = true;
                    break;
                }
            }
            adapter = adapter->Next;
        }
    }

    delete[] reinterpret_cast<BYTE*>(adapterInfo);
    return found;
}

bool getFirstMacAddress(MacAddress& mac) {
    ULONG bufferSize = 15000;
    PIP_ADAPTER_INFO adapterInfo = nullptr;
    PIP_ADAPTER_INFO adapter = nullptr;

    adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new BYTE[bufferSize]);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        delete[] reinterpret_cast<BYTE*>(adapterInfo);
        adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new BYTE[bufferSize]);
    }

    bool found = false;

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        adapter = adapterInfo;

        // Find first non-loopback, non-zero MAC address
        while (adapter) {
            if (adapter->AddressLength == 6) {
                // Check if it's not all zeros
                bool isZero = true;
                for (UINT i = 0; i < 6; ++i) {
                    if (adapter->Address[i] != 0) {
                        isZero = false;
                        break;
                    }
                }

                if (!isZero && adapter->Type == MIB_IF_TYPE_ETHERNET) {
                    std::memcpy(mac.data(), adapter->Address, 6);
                    found = true;
                    break;
                }
            }
            adapter = adapter->Next;
        }
    }

    delete[] reinterpret_cast<BYTE*>(adapterInfo);
    return found;
}

#endif

} // namespace doip

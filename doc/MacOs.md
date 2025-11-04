Yes, macOS uses different approaches for getting MAC addresses. Here are the macOS equivalents:

## The Issue

- `SIOCGIFHWADDR` and `ifr_hwaddr` are Linux-specific
- macOS uses BSD-style networking APIs

## macOS Solutions

### Option 1: Using `getifaddrs()` with `AF_LINK` (Recommended)

```cpp
#include <ifaddrs.h>
#include <net/if_dl.h>

bool getMacAddress(const char* ifname, uint8_t* mac) {
    struct ifaddrs *ifap, *ifaptr;
    
    if (getifaddrs(&ifap) != 0) {
        return false;
    }
    
    for (ifaptr = ifap; ifaptr != nullptr; ifaptr = ifaptr->ifa_next) {
        if (strcmp(ifaptr->ifa_name, ifname) == 0 && 
            ifaptr->ifa_addr->sa_family == AF_LINK) {
            
            struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifaptr->ifa_addr;
            memcpy(mac, LLADDR(sdl), 6);
            freeifaddrs(ifap);
            return true;
        }
    }
    
    freeifaddrs(ifap);
    return false;
}
```

### Option 2: Using `SIOCGIFCONF` + `sysctl`

```cpp
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/route.h>

bool getMacAddress(const char* ifname, uint8_t* mac) {
    int mib[6];
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    mib[5] = if_nametoindex(ifname);
    
    if (mib[5] == 0) return false;
    
    size_t len;
    if (sysctl(mib, 6, nullptr, &len, nullptr, 0) < 0) {
        return false;
    }
    
    char* buf = new char[len];
    if (sysctl(mib, 6, buf, &len, nullptr, 0) < 0) {
        delete[] buf;
        return false;
    }
    
    struct if_msghdr* ifm = (struct if_msghdr*)buf;
    struct sockaddr_dl* sdl = (struct sockaddr_dl*)(ifm + 1);
    memcpy(mac, LLADDR(sdl), 6);
    
    delete[] buf;
    return true;
}
```

## Cross-Platform Approach

For your DoIPServer, I'd recommend:

```cpp
#ifdef __APPLE__
    #include <ifaddrs.h>
    #include <net/if_dl.h>
#else
    #include <linux/if.h>
    #include <sys/ioctl.h>
#endif

// Then in your code:
#ifdef __APPLE__
    // Use getifaddrs() approach
#else
    // Use SIOCGIFHWADDR approach
#endif
```

The `getifaddrs()` approach (Option 1) is cleaner and more portable - it also works on Linux, so you could potentially use it for both platforms if you prefer.
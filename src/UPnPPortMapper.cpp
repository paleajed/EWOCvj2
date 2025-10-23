#include "UPnPPortMapper.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#endif

// Include miniupnpc headers
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

UPnPPortMapper::UPnPPortMapper()
    : upnp_urls_(nullptr), upnp_data_(nullptr), gateway_found_(false) {

    upnp_urls_ = new UPNPUrls();
    upnp_data_ = new IGDdatas();
    memset(upnp_urls_, 0, sizeof(UPNPUrls));
    memset(upnp_data_, 0, sizeof(IGDdatas));
}

UPnPPortMapper::~UPnPPortMapper() {
    cleanup();
}

void UPnPPortMapper::cleanup() {
    if (gateway_found_ && upnp_urls_) {
        FreeUPNPUrls(static_cast<UPNPUrls*>(upnp_urls_));
        gateway_found_ = false;
    }

    if (upnp_urls_) {
        delete static_cast<UPNPUrls*>(upnp_urls_);
        upnp_urls_ = nullptr;
    }

    if (upnp_data_) {
        delete static_cast<IGDdatas*>(upnp_data_);
        upnp_data_ = nullptr;
    }
}

bool UPnPPortMapper::discoverGateway(int timeout_ms) {
    if (gateway_found_) {
        return true;  // Already discovered
    }

    std::cout << "UPnP: Discovering gateway devices..." << std::endl;

    UPNPDev* devlist = nullptr;
    int error = 0;

#if MINIUPNPC_API_VERSION >= 14
    // For miniupnpc >= 1.9
    devlist = upnpDiscover(timeout_ms, nullptr, nullptr, 0, 0, 2, &error);
#else
    // For older versions
    devlist = upnpDiscover(timeout_ms, nullptr, nullptr, 0, 0, &error);
#endif

    if (!devlist) {
        last_error_ = "UPnP: No IGD devices found on network";
        std::cout << last_error_ << std::endl;
        return false;
    }

    std::cout << "UPnP: Devices found, selecting gateway..." << std::endl;

    local_ip_ = getLocalIP();

    char lan_addr[64] = "";
    char wan_addr[64] = "";
    int status = UPNP_GetValidIGD(devlist,
                                   static_cast<UPNPUrls*>(upnp_urls_),
                                   static_cast<IGDdatas*>(upnp_data_),
                                   lan_addr, sizeof(lan_addr),
                                   wan_addr, sizeof(wan_addr));

    freeUPNPDevlist(devlist);

    if (status == 1) {
        std::cout << "UPnP: Found valid IGD: " <<
                     static_cast<UPNPUrls*>(upnp_urls_)->controlURL << std::endl;
        std::cout << "UPnP: Local LAN IP: " << lan_addr << std::endl;
        gateway_found_ = true;
        return true;
    } else if (status == 2) {
        std::cout << "UPnP: Found connected IGD (not root device)" << std::endl;
        gateway_found_ = true;
        return true;
    } else if (status == 3) {
        last_error_ = "UPnP: Found UPnP device but not recognized as IGD";
        std::cout << last_error_ << std::endl;
    } else {
        last_error_ = "UPnP: No valid IGD found";
        std::cout << last_error_ << std::endl;
    }

    return false;
}

bool UPnPPortMapper::addPortMapping(uint16_t external_port,
                                   uint16_t internal_port,
                                   const std::string& protocol,
                                   const std::string& description,
                                   uint32_t lease_duration) {
    if (!gateway_found_) {
        last_error_ = "UPnP: Gateway not discovered. Call discoverGateway() first.";
        std::cout << last_error_ << std::endl;
        return false;
    }

    if (local_ip_.empty()) {
        local_ip_ = getLocalIP();
    }

    std::cout << "UPnP: Adding port mapping - External: " << external_port
              << " -> Internal: " << internal_port << " (" << protocol << ")" << std::endl;

    char ext_port_str[16];
    char int_port_str[16];
    char lease_str[16];

    snprintf(ext_port_str, sizeof(ext_port_str), "%u", external_port);
    snprintf(int_port_str, sizeof(int_port_str), "%u", internal_port);
    snprintf(lease_str, sizeof(lease_str), "%u", lease_duration);

    int result = UPNP_AddPortMapping(
        static_cast<UPNPUrls*>(upnp_urls_)->controlURL,
        static_cast<IGDdatas*>(upnp_data_)->first.servicetype,
        ext_port_str,
        int_port_str,
        local_ip_.c_str(),
        description.c_str(),
        protocol.c_str(),
        nullptr,  // Remote host (nullptr = any)
        lease_str
    );

    if (result == UPNPCOMMAND_SUCCESS) {
        std::cout << "UPnP: Port mapping added successfully!" << std::endl;
        return true;
    } else {
        last_error_ = std::string("UPnP: Failed to add port mapping: ") +
                     strupnperror(result);
        std::cout << last_error_ << std::endl;
        return false;
    }
}

bool UPnPPortMapper::removePortMapping(uint16_t external_port, const std::string& protocol) {
    if (!gateway_found_) {
        last_error_ = "UPnP: Gateway not discovered";
        return false;
    }

    std::cout << "UPnP: Removing port mapping for port " << external_port
              << " (" << protocol << ")" << std::endl;

    char ext_port_str[16];
    snprintf(ext_port_str, sizeof(ext_port_str), "%u", external_port);

    int result = UPNP_DeletePortMapping(
        static_cast<UPNPUrls*>(upnp_urls_)->controlURL,
        static_cast<IGDdatas*>(upnp_data_)->first.servicetype,
        ext_port_str,
        protocol.c_str(),
        nullptr  // Remote host
    );

    if (result == UPNPCOMMAND_SUCCESS) {
        std::cout << "UPnP: Port mapping removed successfully" << std::endl;
        return true;
    } else {
        last_error_ = std::string("UPnP: Failed to remove port mapping: ") +
                     strupnperror(result);
        std::cout << last_error_ << std::endl;
        return false;
    }
}

std::string UPnPPortMapper::getExternalIP() {
    if (!gateway_found_) {
        last_error_ = "UPnP: Gateway not discovered";
        return "";
    }

    char external_ip[40] = "";

    int result = UPNP_GetExternalIPAddress(
        static_cast<UPNPUrls*>(upnp_urls_)->controlURL,
        static_cast<IGDdatas*>(upnp_data_)->first.servicetype,
        external_ip
    );

    if (result == UPNPCOMMAND_SUCCESS) {
        std::cout << "UPnP: External IP address: " << external_ip << std::endl;
        return std::string(external_ip);
    } else {
        last_error_ = std::string("UPnP: Failed to get external IP: ") +
                     strupnperror(result);
        std::cout << last_error_ << std::endl;
        return "";
    }
}

std::string UPnPPortMapper::getLocalIP() {
    std::string local_ip;

#ifdef _WIN32
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname, nullptr, &hints, &result) == 0) {
            struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
            local_ip = inet_ntoa(addr->sin_addr);
            freeaddrinfo(result);
        }
    }
#else
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;

            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN);

                // Skip loopback
                if (strcmp(ip_str, "127.0.0.1") != 0) {
                    local_ip = ip_str;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
#endif

    return local_ip;
}

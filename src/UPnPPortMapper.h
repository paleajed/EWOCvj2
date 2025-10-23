#ifndef UPNPPORTMAPPER_H
#define UPNPPORTMAPPER_H

#include <string>
#include <cstdint>

/**
 * UPnP Port Mapper
 * Automatically configures router port forwarding using UPnP protocol
 * Allows external clients to connect without manual router configuration
 */
class UPnPPortMapper {
public:
    UPnPPortMapper();
    ~UPnPPortMapper();

    /**
     * Discover UPnP-enabled gateway device
     * @param timeout_ms Discovery timeout in milliseconds (default: 2000)
     * @return true if gateway found, false otherwise
     */
    bool discoverGateway(int timeout_ms = 2000);

    /**
     * Add port mapping on the gateway
     * @param external_port External port to open
     * @param internal_port Internal port to forward to
     * @param protocol "TCP" or "UDP"
     * @param description Description for the mapping
     * @param lease_duration Lease duration in seconds (0 = permanent)
     * @return true if successful, false otherwise
     */
    bool addPortMapping(uint16_t external_port,
                       uint16_t internal_port,
                       const std::string& protocol,
                       const std::string& description,
                       uint32_t lease_duration = 0);

    /**
     * Remove port mapping from the gateway
     * @param external_port External port to close
     * @param protocol "TCP" or "UDP"
     * @return true if successful, false otherwise
     */
    bool removePortMapping(uint16_t external_port, const std::string& protocol);

    /**
     * Get external (public) IP address
     * @return External IP address as string, or empty string on failure
     */
    std::string getExternalIP();

    /**
     * Check if UPnP is available and gateway was discovered
     * @return true if UPnP is available
     */
    bool isAvailable() const { return gateway_found_; }

    /**
     * Get last error message
     * @return Error message string
     */
    std::string getLastError() const { return last_error_; }

private:
    void* upnp_urls_;        // UPNPUrls*
    void* upnp_data_;        // IGDdatas*
    std::string local_ip_;
    std::string last_error_;
    bool gateway_found_;

    void cleanup();
    std::string getLocalIP();
};

#endif // UPNPPORTMAPPER_H

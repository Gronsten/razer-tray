#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "ConfigManager.h"

// Structure to hold Razer device information
struct RazerDevice {
    std::wstring name;
    std::wstring instanceId;
    std::optional<int> batteryLevel;  // 0-100, or nullopt if unavailable
    bool isConnected;

    RazerDevice(std::wstring devName, std::wstring devInstanceId)
        : name(std::move(devName))
        , instanceId(std::move(devInstanceId))
        , batteryLevel(std::nullopt)
        , isConnected(false)
    {}
};

class DeviceMonitor {
public:
    DeviceMonitor();
    explicit DeviceMonitor(const Config& config);
    ~DeviceMonitor();

    // Enumerate all Razer Bluetooth LE devices (uses config if available)
    std::vector<std::unique_ptr<RazerDevice>> enumerateRazerDevices();

    // Update battery levels and connection status for devices
    void updateDeviceInfo(std::vector<std::unique_ptr<RazerDevice>>& devices);

private:
    // Query battery level for a specific device
    std::optional<int> getBatteryLevel(const std::wstring& instanceId);

    // Check if device is actually connected (not just paired)
    bool isDeviceConnected(const std::wstring& instanceId);

    // Get device node instance from instance ID
    bool getDeviceNode(const std::wstring& instanceId, DWORD& devInst);

    // Optional config (if not set, uses default hardcoded patterns)
    std::optional<Config> config;
};

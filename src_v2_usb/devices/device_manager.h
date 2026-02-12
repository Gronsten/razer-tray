#pragma once

#include <vector>
#include <memory>
#include <map>
#include <libusb.h>
#include "razer_device.h"

namespace razer {

// Razer vendor ID
constexpr uint16_t RAZER_VID = 0x1532;

// Known device database (PID -> Name mapping)
struct KnownDevice {
    uint16_t pid;
    const char* name;
    const char* type;  // "mouse", "keyboard", "mousemat", "headset", "accessory"
};

// Device manager - handles discovery and lifecycle
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // Disable copy, allow move
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    // Initialize libusb
    bool initialize();

    // Discover all connected Razer devices
    std::vector<DeviceInfo> discover_devices();

    // Create device instance from info
    std::unique_ptr<RazerDevice> create_device(const DeviceInfo& info);

    // Get device name from PID (from known device database)
    static const char* get_device_name(uint16_t pid);
    static const char* get_device_type(uint16_t pid);

private:
    libusb_context* context;

    // Read device info from libusb device
    DeviceInfo read_device_info(libusb_device* device);

    // Read string descriptor
    std::string read_string_descriptor(libusb_device_handle* handle, uint8_t index);
};

// Known Razer devices database
extern const KnownDevice KNOWN_DEVICES[];
extern const size_t KNOWN_DEVICES_COUNT;

} // namespace razer

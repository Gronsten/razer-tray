#pragma once

#include <string>
#include <memory>
#include <optional>
#include <libusb.h>
#include "../core/razer_protocol.h"

namespace razer {

// Battery status structure
struct BatteryStatus {
    int percentage;      // 0-100
    bool is_charging;    // true if charging
    bool is_valid;       // true if data was successfully read

    BatteryStatus() : percentage(0), is_charging(false), is_valid(false) {}
    BatteryStatus(int pct, bool charging)
        : percentage(pct), is_charging(charging), is_valid(true) {}
};

// Device information
struct DeviceInfo {
    uint16_t vid;
    uint16_t pid;
    std::string serial;
    std::string product_name;
    std::string manufacturer;
    uint8_t transaction_id;  // Device-specific transaction ID

    DeviceInfo() : vid(0), pid(0), transaction_id(0x1f) {}
};

// Razer device class
class RazerDevice {
public:
    RazerDevice(libusb_device* device, const DeviceInfo& info);
    ~RazerDevice();

    // Disable copy, allow move
    RazerDevice(const RazerDevice&) = delete;
    RazerDevice& operator=(const RazerDevice&) = delete;
    RazerDevice(RazerDevice&&) = default;
    RazerDevice& operator=(RazerDevice&&) = default;

    // Device operations
    bool open();
    void close();
    bool is_open() const { return handle != nullptr; }

    // Battery operations
    std::optional<BatteryStatus> get_battery_status();

    // Device info accessors
    const DeviceInfo& info() const { return device_info; }
    uint16_t vid() const { return device_info.vid; }
    uint16_t pid() const { return device_info.pid; }
    const std::string& serial() const { return device_info.serial; }
    const std::string& product_name() const { return device_info.product_name; }

    // Display name for UI
    std::string display_name() const;

private:
    libusb_device* device;
    libusb_device_handle* handle;
    DeviceInfo device_info;
    bool kernel_driver_detached;
    int interface_number;

    // USB control transfer helpers
    bool send_report(const Report& report, int timeout_ms = 1000);
    std::optional<Report> receive_report(int timeout_ms = 1000);

    // Auto-detect transaction ID for device
    uint8_t detect_transaction_id();
};

} // namespace razer

#include "razer_device.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace razer {

RazerDevice::RazerDevice(libusb_device* device, const DeviceInfo& info)
    : device(device)
    , handle(nullptr)
    , device_info(info)
    , kernel_driver_detached(false)
    , interface_number(0)  // Use interface 0 (works for Mouse Dock Pro)
{
    libusb_ref_device(device);
}

RazerDevice::~RazerDevice() {
    close();
    if (device) {
        libusb_unref_device(device);
    }
}

bool RazerDevice::open() {
    if (handle) {
        return true;  // Already open
    }

    int result = libusb_open(device, &handle);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to open device: " << libusb_error_name(result) << std::endl;
        return false;
    }

    // Detach kernel driver if active (Linux only, harmless on Windows)
    if (libusb_kernel_driver_active(handle, interface_number) == 1) {
        result = libusb_detach_kernel_driver(handle, interface_number);
        if (result == LIBUSB_SUCCESS) {
            kernel_driver_detached = true;
        }
    }

    // Claim interface
    result = libusb_claim_interface(handle, interface_number);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to claim interface: " << libusb_error_name(result) << std::endl;
        libusb_close(handle);
        handle = nullptr;
        return false;
    }

    return true;
}

void RazerDevice::close() {
    if (!handle) {
        return;
    }

    // Release interface
    libusb_release_interface(handle, interface_number);

    // Reattach kernel driver if we detached it
    if (kernel_driver_detached) {
        libusb_attach_kernel_driver(handle, interface_number);
        kernel_driver_detached = false;
    }

    libusb_close(handle);
    handle = nullptr;
}

std::optional<BatteryStatus> RazerDevice::get_battery_status() {
    if (!handle) {
        if (!open()) {
            return std::nullopt;
        }
    }

    // Auto-detect transaction ID if not set
    if (device_info.transaction_id == 0x1f) {
        device_info.transaction_id = detect_transaction_id();
    }

    // Query battery level
    Report battery_query = create_battery_query(device_info.transaction_id);
    if (!send_report(battery_query)) {
        return std::nullopt;
    }

    // Wait for device to process (openrazer uses 600-800 microseconds)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto battery_response = receive_report();
    if (!battery_response) {
        return std::nullopt;
    }

    // Debug the response
    const Report& resp = battery_response.value();
    std::cout << "[DEBUG] Battery response: status=0x" << std::hex << static_cast<int>(resp.status)
              << " data_size=" << std::dec << static_cast<int>(resp.data_size)
              << " args[1]=" << static_cast<int>(resp.arguments[1]) << std::endl;

    int percentage = get_battery_percent(&battery_response.value());
    if (percentage < 0) {
        std::cerr << "[ERROR] Failed to extract battery percentage" << std::endl;
        return std::nullopt;
    }

    std::cout << "[DEBUG] Battery percentage: " << percentage << "%" << std::endl;

    // TODO: Query charging status (disabled for now - needs investigation)
    // The charging status query seems to interfere with subsequent battery reads
    bool is_charging = false;

    return BatteryStatus(percentage, is_charging);
}

std::string RazerDevice::display_name() const {
    std::ostringstream oss;
    if (!device_info.product_name.empty()) {
        oss << device_info.product_name;
    } else {
        oss << "Razer Device";
    }

    if (!device_info.serial.empty()) {
        oss << " (" << device_info.serial.substr(0, 8) << "...)";
    } else {
        oss << " (" << std::hex << std::setw(4) << std::setfill('0') << device_info.pid << ")";
    }

    return oss.str();
}

bool RazerDevice::send_report(const Report& report, int timeout_ms) {
    if (!handle) {
        return false;
    }

    int result = libusb_control_transfer(
        handle,
        REQUEST_TYPE_OUT,
        USB_REQ_SET_REPORT,
        REPORT_VALUE,
        REPORT_INDEX,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(&report)),
        sizeof(Report),
        timeout_ms
    );

    if (result < 0) {
        std::cerr << "Failed to send report: " << libusb_error_name(result) << std::endl;
        return false;
    }

    return true;
}

std::optional<Report> RazerDevice::receive_report(int timeout_ms) {
    if (!handle) {
        return std::nullopt;
    }

    Report response;
    int result = libusb_control_transfer(
        handle,
        REQUEST_TYPE_IN,
        USB_REQ_GET_REPORT,
        REPORT_VALUE,
        REPORT_INDEX,
        reinterpret_cast<unsigned char*>(&response),
        sizeof(Report),
        timeout_ms
    );

    if (result < 0) {
        std::cerr << "Failed to receive report: " << libusb_error_name(result) << std::endl;
        return std::nullopt;
    }

    return response;
}

uint8_t RazerDevice::detect_transaction_id() {
    // Common transaction IDs: 0x1f, 0x3f, 0x9f, 0xff
    // Try each until one works
    const uint8_t ids[] = {0x1f, 0x3f, 0x9f, 0xff};

    for (uint8_t id : ids) {
        Report query = create_battery_query(id);
        if (send_report(query)) {
            // Wait for device to process
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto response = receive_report();
            if (response) {
                // Check for successful status (0x02) or valid battery data
                uint8_t status = response->status;
                int battery = get_battery_raw(&response.value());

                if (status == 0x02 && battery >= 0) {
                    return id;
                }
            }
        }
    }

    return 0x1f;  // Default fallback
}

} // namespace razer

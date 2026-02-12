#include "device_manager.h"
#include <iostream>
#include <algorithm>

namespace razer {

// Known Razer devices database (subset - add more as needed)
const KnownDevice KNOWN_DEVICES[] = {
    // Mice
    {0x00A4, "Razer Mouse Dock Pro", "accessory"},
    {0x00D6, "Razer Basilisk V3 Pro (Wired)", "mouse"},
    {0x00D7, "Razer Basilisk V3 Pro (Wireless)", "mouse"},
    {0x0084, "Razer Basilisk V3", "mouse"},
    {0x0088, "Razer Basilisk V3 X HyperSpeed", "mouse"},
    {0x008C, "Razer DeathAdder V3 Pro (Wired)", "mouse"},
    {0x008D, "Razer DeathAdder V3 Pro (Wireless)", "mouse"},
    {0x0098, "Razer Viper V3 Pro (Wired)", "mouse"},
    {0x009A, "Razer Viper V3 Pro (Wireless)", "mouse"},

    // Keyboards
    {0x024E, "Razer BlackWidow V3", "keyboard"},
    {0x0241, "Razer BlackWidow V3 Pro (Wired)", "keyboard"},
    {0x0258, "Razer BlackWidow V3 Pro (Wireless)", "keyboard"},
    {0x026D, "Razer BlackWidow V4 Pro", "keyboard"},

    // Mousemats
    {0x0C3B, "Razer Firefly V2 Pro", "mousemat"},
    {0x0C3C, "Razer Firefly V2", "mousemat"},

    // Headsets
    {0x0527, "Razer BlackShark V2 Pro (2023)", "headset"},
    {0x0510, "Razer Kraken V3 Pro", "headset"},
};

const size_t KNOWN_DEVICES_COUNT = sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]);

DeviceManager::DeviceManager() : context(nullptr) {}

DeviceManager::~DeviceManager() {
    if (context) {
        libusb_exit(context);
        context = nullptr;
    }
}

bool DeviceManager::initialize() {
    int result = libusb_init(&context);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to initialize libusb: " << libusb_error_name(result) << std::endl;
        return false;
    }

    // Set debug level (0 = no debug, 3 = verbose)
    #ifdef DEBUG
    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    #endif

    return true;
}

std::vector<DeviceInfo> DeviceManager::discover_devices() {
    std::vector<DeviceInfo> devices;

    if (!context) {
        std::cerr << "DeviceManager not initialized" << std::endl;
        return devices;
    }

    libusb_device** device_list = nullptr;
    ssize_t count = libusb_get_device_list(context, &device_list);

    if (count < 0) {
        std::cerr << "Failed to get device list: " << libusb_error_name(count) << std::endl;
        return devices;
    }

    // Scan all USB devices
    for (ssize_t i = 0; i < count; i++) {
        libusb_device* device = device_list[i];
        libusb_device_descriptor desc;

        int result = libusb_get_device_descriptor(device, &desc);
        if (result != LIBUSB_SUCCESS) {
            continue;
        }

        // Filter by Razer VID
        if (desc.idVendor != RAZER_VID) {
            continue;
        }

        // Read device info
        DeviceInfo info = read_device_info(device);
        if (info.vid == RAZER_VID) {
            devices.push_back(info);
        }
    }

    libusb_free_device_list(device_list, 1);

    return devices;
}

std::unique_ptr<RazerDevice> DeviceManager::create_device(const DeviceInfo& info) {
    if (!context) {
        return nullptr;
    }

    libusb_device** device_list = nullptr;
    ssize_t count = libusb_get_device_list(context, &device_list);

    if (count < 0) {
        return nullptr;
    }

    std::unique_ptr<RazerDevice> razer_device;

    // Find matching device
    for (ssize_t i = 0; i < count; i++) {
        libusb_device* device = device_list[i];
        DeviceInfo current_info = read_device_info(device);

        if (current_info.vid == info.vid &&
            current_info.pid == info.pid &&
            current_info.serial == info.serial) {

            razer_device = std::make_unique<RazerDevice>(device, info);
            break;
        }
    }

    libusb_free_device_list(device_list, 1);
    return razer_device;
}

DeviceInfo DeviceManager::read_device_info(libusb_device* device) {
    DeviceInfo info;

    libusb_device_descriptor desc;
    int result = libusb_get_device_descriptor(device, &desc);
    if (result != LIBUSB_SUCCESS) {
        return info;
    }

    info.vid = desc.idVendor;
    info.pid = desc.idProduct;

    // Try to open device to read strings
    libusb_device_handle* handle = nullptr;
    result = libusb_open(device, &handle);
    if (result != LIBUSB_SUCCESS) {
        // Can't open - use defaults
        info.product_name = get_device_name(info.pid);
        info.manufacturer = "Razer Inc.";
        return info;
    }

    // Read serial number
    if (desc.iSerialNumber > 0) {
        info.serial = read_string_descriptor(handle, desc.iSerialNumber);
    }

    // Read product name
    if (desc.iProduct > 0) {
        info.product_name = read_string_descriptor(handle, desc.iProduct);
    }

    // Read manufacturer
    if (desc.iManufacturer > 0) {
        info.manufacturer = read_string_descriptor(handle, desc.iManufacturer);
    }

    // If product name is empty, use known device database
    if (info.product_name.empty()) {
        info.product_name = get_device_name(info.pid);
    }

    libusb_close(handle);
    return info;
}

std::string DeviceManager::read_string_descriptor(libusb_device_handle* handle, uint8_t index) {
    unsigned char buffer[256];
    int result = libusb_get_string_descriptor_ascii(handle, index, buffer, sizeof(buffer));

    if (result > 0) {
        return std::string(reinterpret_cast<char*>(buffer), result);
    }

    return "";
}

const char* DeviceManager::get_device_name(uint16_t pid) {
    for (size_t i = 0; i < KNOWN_DEVICES_COUNT; i++) {
        if (KNOWN_DEVICES[i].pid == pid) {
            return KNOWN_DEVICES[i].name;
        }
    }
    return "Unknown Razer Device";
}

const char* DeviceManager::get_device_type(uint16_t pid) {
    for (size_t i = 0; i < KNOWN_DEVICES_COUNT; i++) {
        if (KNOWN_DEVICES[i].pid == pid) {
            return KNOWN_DEVICES[i].type;
        }
    }
    return "unknown";
}

} // namespace razer

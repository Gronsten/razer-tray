#include "DeviceMonitor.h"
#include "SafeHandles.h"
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devpkey.h>
#include <initguid.h>
#include <vector>
#include <string>

// Battery level property key: {104EA319-6EE2-4701-BD47-8DDBF425BBE5} 2
DEFINE_GUID(GUID_BATTERY_LEVEL,
    0x104EA319, 0x6EE2, 0x4701, 0xBD, 0x47, 0x8D, 0xDB, 0xF4, 0x25, 0xBB, 0xE5);

const DEVPROPKEY DEVPKEY_Device_BatteryLevel = {
    GUID_BATTERY_LEVEL,
    2  // Property ID
};

// Connection status property key: DEVPKEY_Device_IsConnected
// GUID: {83da6326-97a6-4088-9453-a1923f573b29}, Property ID: 15
DEFINE_GUID(GUID_DEVICE_ISCONNECTED,
    0x83da6326, 0x97a6, 0x4088, 0x94, 0x53, 0xa1, 0x92, 0x3f, 0x57, 0x3b, 0x29);

const DEVPROPKEY DEVPKEY_Device_IsConnected_Custom = {
    GUID_DEVICE_ISCONNECTED,
    15  // Property ID (correct value!)
};

DeviceMonitor::DeviceMonitor() : config(std::nullopt) {
    // Constructor - initialization if needed (no config = use default hardcoded patterns)
}

DeviceMonitor::DeviceMonitor(const Config& cfg) : config(cfg) {
    // Constructor with config
}

DeviceMonitor::~DeviceMonitor() {
    // Destructor - cleanup handled by RAII
}

std::vector<std::unique_ptr<RazerDevice>> DeviceMonitor::enumerateRazerDevices() {
    std::vector<std::unique_ptr<RazerDevice>> devices;

    // Get device information set for Bluetooth devices
    DeviceInfoHandle deviceInfo(
        SetupDiGetClassDevsW(
            nullptr,
            L"BTHLE",  // Bluetooth LE enumerator
            nullptr,
            DIGCF_ALLCLASSES | DIGCF_PRESENT
        )
    );

    if (!deviceInfo.isValid()) {
        return devices;  // Return empty vector on failure
    }

    SP_DEVINFO_DATA deviceInfoData = {};
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Enumerate all devices
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo.get(), i, &deviceInfoData); ++i) {
        // Get device instance ID
        WCHAR instanceId[MAX_PATH] = {};
        if (!SetupDiGetDeviceInstanceIdW(
                deviceInfo.get(),
                &deviceInfoData,
                instanceId,
                MAX_PATH,
                nullptr)) {
            continue;
        }

        // Get device description/name
        WCHAR deviceName[256] = {};
        DWORD propertyType = 0;
        if (!SetupDiGetDeviceRegistryPropertyW(
                deviceInfo.get(),
                &deviceInfoData,
                SPDRP_FRIENDLYNAME,
                &propertyType,
                reinterpret_cast<PBYTE>(deviceName),
                sizeof(deviceName),
                nullptr)) {
            continue;
        }

        // Filter for Razer devices
        std::wstring name(deviceName);
        std::wstring instId(instanceId);

        // Check if device is BTHLE
        if (instId.find(L"BTHLE\\") != 0) {
            continue;
        }

        bool matches = false;

        // Use config patterns if available, otherwise use default hardcoded patterns
        if (config.has_value()) {
            ConfigManager configMgr;
            matches = configMgr.matchesDevicePatterns(name, config.value());
        } else {
            // Default hardcoded patterns (for backward compatibility)
            matches = (name.find(L"BSK") != std::wstring::npos ||
                      name.find(L"Razer") != std::wstring::npos ||
                      name.find(L"razer") != std::wstring::npos);
        }

        if (matches) {
            devices.push_back(std::make_unique<RazerDevice>(name, instId));
        }
    }

    return devices;
}

bool DeviceMonitor::getDeviceNode(const std::wstring& instanceId, DWORD& devInst) {
    // Convert instance ID to device node
    CONFIGRET ret = CM_Locate_DevNodeW(
        &devInst,
        const_cast<DEVINSTID_W>(instanceId.c_str()),
        CM_LOCATE_DEVNODE_NORMAL
    );

    return ret == CR_SUCCESS;
}

std::optional<int> DeviceMonitor::getBatteryLevel(const std::wstring& instanceId) {
    DWORD devInst = 0;
    if (!getDeviceNode(instanceId, devInst)) {
        return std::nullopt;
    }

    // Query battery level property
    BYTE buffer[256] = {};
    ULONG bufferSize = sizeof(buffer);
    DEVPROPTYPE propertyType = 0;

    CONFIGRET ret = CM_Get_DevNode_PropertyW(
        devInst,
        &DEVPKEY_Device_BatteryLevel,
        &propertyType,
        buffer,
        &bufferSize,
        0
    );

    if (ret != CR_SUCCESS || propertyType != DEVPROP_TYPE_BYTE) {
        return std::nullopt;
    }

    // Battery level is returned as a byte (0-100)
    int batteryLevel = static_cast<int>(buffer[0]);
    if (batteryLevel >= 0 && batteryLevel <= 100) {
        return batteryLevel;
    }

    return std::nullopt;
}

bool DeviceMonitor::isDeviceConnected(const std::wstring& instanceId) {
    DWORD devInst = 0;
    if (!getDeviceNode(instanceId, devInst)) {
        return false;
    }

    // Query connection status property
    BYTE buffer[256] = {};
    ULONG bufferSize = sizeof(buffer);
    DEVPROPTYPE propertyType = 0;

    CONFIGRET ret = CM_Get_DevNode_PropertyW(
        devInst,
        &DEVPKEY_Device_IsConnected_Custom,
        &propertyType,
        buffer,
        &bufferSize,
        0
    );

    if (ret != CR_SUCCESS || propertyType != DEVPROP_TYPE_BOOLEAN) {
        return false;
    }

    // Connection status is returned as DEVPROP_BOOLEAN (DEVPROP_TRUE/DEVPROP_FALSE)
    DEVPROP_BOOLEAN isConnected = *reinterpret_cast<DEVPROP_BOOLEAN*>(buffer);
    return isConnected == DEVPROP_TRUE;
}

void DeviceMonitor::updateDeviceInfo(std::vector<std::unique_ptr<RazerDevice>>& devices) {
    for (auto& device : devices) {
        device->batteryLevel = getBatteryLevel(device->instanceId);
        device->isConnected = isDeviceConnected(device->instanceId);
    }
}

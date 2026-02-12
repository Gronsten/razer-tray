#pragma once

#include <cstdint>
#include <cstring>

// Razer USB Protocol Implementation
// Based on openrazer driver analysis

namespace razer {

// Command classes
constexpr uint8_t CLASS_MISC = 0x07;

// Command IDs
constexpr uint8_t CMD_GET_BATTERY = 0x80;
constexpr uint8_t CMD_GET_CHARGING_STATUS = 0x84;

// USB Control Transfer parameters
constexpr uint8_t REQUEST_TYPE_OUT = 0x21;  // Host to Device, Class, Interface
constexpr uint8_t REQUEST_TYPE_IN = 0xA1;   // Device to Host, Class, Interface
constexpr uint8_t USB_REQ_SET_REPORT = 0x09;
constexpr uint8_t USB_REQ_GET_REPORT = 0x01;
constexpr uint16_t REPORT_VALUE = 0x300;
constexpr uint16_t REPORT_INDEX = 0x02;

// 90-byte Razer report structure
#pragma pack(push, 1)
struct Report {
    uint8_t status;                 // 0x00 for new command
    uint8_t transaction_id;         // Transaction ID (device-specific: 0x1f, 0x3f, 0x9f, 0xff)
    uint16_t remaining_packets;     // Big Endian, usually 0x0000
    uint8_t protocol_type;          // Always 0x00
    uint8_t data_size;              // Size of arguments payload (max 80)
    uint8_t command_class;          // Command category
    uint8_t command_id;             // Command identifier
    uint8_t arguments[80];          // Command payload
    uint8_t crc;                    // XOR checksum (bytes 2-87)
    uint8_t reserved;               // Always 0x00

    Report() {
        std::memset(this, 0, sizeof(Report));
    }
};
#pragma pack(pop)

static_assert(sizeof(Report) == 90, "Report must be 90 bytes");

// Calculate CRC (XOR of bytes 2-87)
inline uint8_t calculate_crc(const Report* report) {
    uint8_t crc = 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(report);
    for (int i = 2; i < 88; i++) {
        crc ^= data[i];
    }
    return crc;
}

// Create battery query command
inline Report create_battery_query(uint8_t transaction_id = 0x1f) {
    Report report;
    report.status = 0x00;
    report.transaction_id = transaction_id;
    report.data_size = 0x02;
    report.command_class = CLASS_MISC;
    report.command_id = CMD_GET_BATTERY;
    report.crc = calculate_crc(&report);
    return report;
}

// Create charging status query command
inline Report create_charging_query(uint8_t transaction_id = 0x1f) {
    Report report;
    report.status = 0x00;
    report.transaction_id = transaction_id;
    report.data_size = 0x02;
    report.command_class = CLASS_MISC;
    report.command_id = CMD_GET_CHARGING_STATUS;
    report.crc = calculate_crc(&report);
    return report;
}

// Extract battery level from response (0-255 scale)
inline int get_battery_raw(const Report* response) {
    // Check for successful status (0x02 = SUCCESS)
    if (response->status != 0x02) {
        return -1;
    }

    if (response->data_size < 2) {
        return -1;
    }

    // Battery level is in arguments[1]
    return response->arguments[1];
}

// Convert battery level to percentage (0-100)
inline int get_battery_percent(const Report* response) {
    int raw = get_battery_raw(response);
    if (raw < 0) return -1;
    return (raw * 100) / 255;
}

// Extract charging status from response
inline bool get_charging_status(const Report* response) {
    if (response->command_class == CLASS_MISC &&
        response->command_id == CMD_GET_CHARGING_STATUS &&
        response->data_size >= 2) {
        return response->arguments[1] == 0x01;
    }
    return false;
}

} // namespace razer

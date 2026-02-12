# Razer Tray v2.0 - Architecture Documentation

**Last Updated:** 2026-02-12
**Version:** 2.0.0-dev (in development, not yet committed to git)

---

## Overview

Razer Tray v2.0 is a complete rewrite from the v1.0 Bluetooth LE approach. It uses **libusb** for direct USB communication with Razer devices via the reverse-engineered Razer USB protocol (based on [openrazer](https://github.com/openrazer/openrazer)). This enables USB-connected devices, 2.4GHz wireless dongles, and opens the door to RGB/DPI control.

### Key Differences from v1.0

| Aspect | v1.0 (Bluetooth LE) | v2.0 (USB/libusb) |
|--------|---------------------|--------------------|
| Communication | Windows BT GATT APIs | libusb USB control transfers |
| Device Discovery | SetupAPI + BTHLE subsystem | libusb device enumeration by VID |
| Protocol | Windows property keys | 90-byte Razer USB HID reports |
| Capabilities | Battery only | Battery + future RGB/DPI/polling |
| Dependencies | None (pure Win32) | libusb-1.0.29 (static linked) |
| Config Tool | PowerShell (`razer-config.ps1`) | Built-in CLI menu + auto-discovery |

---

## Project Structure

```
razer-tray/
├── src_v2_usb/                    # v2.0 source code (ACTIVE DEVELOPMENT)
│   ├── main.cpp                   # Entry point, CLI switch routing
│   ├── core/
│   │   └── razer_protocol.h       # Razer USB protocol (90-byte reports, CRC)
│   ├── devices/
│   │   ├── device_manager.h       # DeviceManager class declaration
│   │   ├── device_manager.cpp     # libusb init, device discovery, known device DB
│   │   ├── razer_device.h         # RazerDevice class, BatteryStatus, DeviceInfo structs
│   │   └── razer_device.cpp       # USB control transfers, battery queries, transaction ID detection
│   └── ui/
│       ├── tray_app.h             # TrayApp class declaration
│       ├── tray_app.cpp           # Win32 system tray, battery icon rendering, context menu
│       ├── cli_menu.h             # CLIMenu class declaration
│       └── cli_menu.cpp           # Interactive terminal menu system
├── archive/
│   └── v1.0/                      # Archived v1.0 Bluetooth LE source (complete, not deleted)
├── build_v2.bat                   # Build script (MinGW64 + libusb static)
├── build_v2/bin/razer-tray.exe    # Compiled v2.0 binary (295KB)
├── RESEARCH.md                    # openrazer analysis, feature catalog, phase plan
├── ARCHITECTURE.md                # This file
├── CHANGELOG.md                   # Release history
└── README.md                      # User docs (still describes v1.0 - needs update for v2.0)
```

### External Dependency

- **libusb-1.0.29** at `C:\AppInstall\dev\libusb-1.0.29\`
  - Headers: `include/libusb.h`
  - Static lib: `MinGW64/static/libusb-1.0.a`
  - Linked statically into the executable (no DLL needed at runtime)

---

## Component Reference

### 1. Entry Point — `main.cpp`

**Purpose:** CLI argument parsing and mode dispatch.

| Mode | Switch | Function | Status |
|------|--------|----------|--------|
| Tray (default) | `--tray` or no args | `run_tray()` | Working (skeleton) |
| Discovery | `--discover`, `-d` | `run_discovery()` | Working |
| Battery Test | `--test-battery` | `run_test_battery()` | Working |
| Device List | `--list`, `-l` | `run_list()` | Stub ("not yet implemented") |
| Interactive Menu | `--menu`, `-m` | `CLIMenu::run()` | Working |
| Version | `--version`, `-v` | `print_version()` | Working |
| Help | `--help`, `-h` | `print_help()` | Working |

### 2. Razer Protocol — `core/razer_protocol.h`

**Purpose:** Header-only implementation of the Razer USB HID protocol.

Key components:
- **`Report` struct** (90 bytes, packed): `status`, `transaction_id`, `remaining_packets`, `protocol_type`, `data_size`, `command_class`, `command_id`, `arguments[80]`, `crc`, `reserved`
- **`calculate_crc()`**: XOR checksum of bytes 2-87
- **`create_battery_query()`**: Builds a battery level request (class `0x07`, cmd `0x80`)
- **`create_charging_query()`**: Builds a charging status request (class `0x07`, cmd `0x84`)
- **`get_battery_percent()`**: Extracts battery % from response (raw 0-255 scaled to 0-100)
- **`get_charging_status()`**: Extracts charging boolean from response

USB control transfer parameters:
- OUT: `bmRequestType=0x21`, `bRequest=0x09` (SET_REPORT), `wValue=0x300`, `wIndex=0x02`
- IN: `bmRequestType=0xA1`, `bRequest=0x01` (GET_REPORT), same value/index

### 3. Device Manager — `devices/device_manager.cpp/h`

**Purpose:** libusb lifecycle management, device discovery, known device database.

Key methods:
- **`initialize()`**: Calls `libusb_init()`, sets debug level in DEBUG builds
- **`discover_devices()`**: Enumerates all USB devices, filters by `VID=0x1532`, reads descriptors
- **`create_device()`**: Re-enumerates to find matching device by VID+PID+serial, returns `unique_ptr<RazerDevice>`
- **`read_device_info()`**: Opens device temporarily to read serial, product name, manufacturer strings
- **`get_device_name()`** / **`get_device_type()`**: Static lookups against `KNOWN_DEVICES[]` array

Known device database (17 entries currently):
- 9 mice (Basilisk V3 variants, DeathAdder V3 Pro, Viper V3 Pro)
- 4 keyboards (BlackWidow V3/V4)
- 2 mousemats (Firefly V2/V2 Pro)
- 2 headsets (BlackShark V2 Pro, Kraken V3 Pro)

### 4. Razer Device — `devices/razer_device.cpp/h`

**Purpose:** Individual device control via USB control transfers.

Key methods:
- **`open()`**: Opens device handle, detaches kernel driver if needed, claims interface 0
- **`close()`**: Releases interface, reattaches kernel driver, closes handle
- **`get_battery_status()`**: Sends battery query, waits 10ms, reads response, returns `optional<BatteryStatus>`
- **`detect_transaction_id()`**: Tries IDs `{0x1f, 0x3f, 0x9f, 0xff}` sequentially until one returns valid battery data
- **`send_report()`** / **`receive_report()`**: USB control transfer wrappers

Important notes:
- Uses `interface_number = 0` (confirmed working for Mouse Dock Pro)
- Charging status query is **disabled** (line 116-118) — commented as "needs investigation", may interfere with subsequent reads
- Debug output (`[DEBUG]` prefixed) is still active in `get_battery_status()` — should be removed or made conditional before release

### 5. Tray App — `ui/tray_app.cpp/h`

**Purpose:** Windows system tray integration with battery icon.

Architecture:
- Creates a message-only window (`HWND_MESSAGE`) for the Win32 message pump
- Registers `Shell_NotifyIconW` with custom `WM_TRAYICON` callback
- Renders 16x16 RGBA battery icon via `BITMAPV5HEADER` + direct pixel manipulation
- 5-minute auto-refresh timer (`SetTimer` with `TIMER_REFRESH`)

Tray features implemented:
- Battery-shaped icon with color coding (green/yellow/orange/red based on %)
- Tooltip showing device name + battery % + charging status
- Right-click context menu: device info (disabled text), "Refresh Now", "Exit"
- Double-click to refresh
- Multi-device display in menu (shows all discovered devices)
- Special handling for Mouse Dock Pro (`PID 0x00A4`) — displays as "Razer Mouse"

### 6. CLI Menu — `ui/cli_menu.cpp/h`

**Purpose:** Interactive terminal-based menu for device management.

Menu structure:
1. Discover Devices — runs libusb scan, shows VID:PID, serial, type
2. List Configured Devices — stub ("not yet implemented")
3. Test Battery Reading — queries battery on all found devices with visual bar
4. Start System Tray Monitor — stub ("not yet implemented")
5. About — version info and feature list
0. Exit

Note: Menu has a duplicate `[0] Exit` line (line 37 in `cli_menu.cpp`).

---

## Execution Flow

### Default Launch (Tray Mode)

```
main() → run_tray(hInstance)
  → TrayApp::initialize()
    → DeviceManager::initialize()         # libusb_init
    → TrayApp::discover_devices()         # Scan USB, filter VID 0x1532
      → DeviceManager::discover_devices()
      → DeviceManager::create_device()    # For each found device
    → register_window_class()             # WNDCLASS for message pump
    → create_window()                     # HWND_MESSAGE hidden window
    → create_tray_icon()                  # Shell_NotifyIconW
    → SetTimer(REFRESH_INTERVAL)          # 5-minute auto-refresh
    → refresh_battery_status()            # Initial battery read
  → TrayApp::run()                        # GetMessage() loop (blocks)
```

### Battery Query Flow

```
RazerDevice::get_battery_status()
  → detect_transaction_id()              # Try 0x1f, 0x3f, 0x9f, 0xff
    → create_battery_query(id)           # Build 90-byte report
    → send_report()                      # libusb_control_transfer (OUT)
    → sleep 10ms                         # Wait for device processing
    → receive_report()                   # libusb_control_transfer (IN)
    → Check status==0x02 && battery>=0   # Validate response
  → create_battery_query(detected_id)    # Use detected transaction ID
  → send_report()
  → sleep 10ms
  → receive_report()
  → get_battery_percent()                # raw*100/255
  → return BatteryStatus(percentage, is_charging=false)
```

---

## Build System

### Build Command

```batch
build_v2.bat
```

### Compiler Configuration

- **Compiler:** g++ (MinGW-w64)
- **Standard:** C++20
- **Flags:** `-Wall -Wextra -O2`
- **Static libraries:** `libusb-1.0.a`, `setupapi`, `ole32`, `advapi32`, `gdi32`, `shell32`, `user32`
- **Output:** `build_v2/bin/razer-tray.exe` (295KB)

### Source Files Compiled

1. `src_v2_usb/main.cpp`
2. `src_v2_usb/devices/razer_device.cpp`
3. `src_v2_usb/devices/device_manager.cpp`
4. `src_v2_usb/ui/cli_menu.cpp`
5. `src_v2_usb/ui/tray_app.cpp`

---

## Git Status (as of 2026-02-12)

**Branch:** `main` (3 commits, all from v1.0 release)

**Uncommitted state:**
- v1.0 source files show as "deleted" (moved to `archive/v1.0/`)
- All v2.0 files are untracked: `RESEARCH.md`, `archive/`, `build_v2.bat`, `src_v2_usb/`
- **No v2.0 code has been committed yet**

**Branches:**
- `main` — active development
- `archive/bluetooth-version` — v1.0 Bluetooth LE snapshot

---

## Known Issues & Technical Debt

1. **Charging status disabled** — `razer_device.cpp:116-118`: The charging query (`CMD 0x84`) was disabled because it "seems to interfere with subsequent battery reads." Needs investigation — possibly a timing issue or wrong interface.

2. **Debug output in production path** — `razer_device.cpp:104-114`: `[DEBUG]` lines in `get_battery_status()` print to stdout during tray mode. Should be removed or gated behind a `--verbose` flag.

3. **Duplicate menu item** — `cli_menu.cpp:37`: `[0] Exit` appears twice in the main menu.

4. **Transaction ID re-detection** — `razer_device.cpp:84-86`: The `if (transaction_id == 0x1f)` check means detect runs every call since `0x1f` is also the default. Should cache after first successful detection.

5. **No WinUSB driver check** — If the Razer device doesn't have WinUSB installed (requires Zadig for first-time setup), `libusb_open()` fails silently with a generic error. Should provide user-friendly guidance.

6. **String encoding** — `tray_app.cpp:271,359`: ASCII-to-wide string conversion (`wstring(name.begin(), name.end())`) only works for ASCII device names. Should use proper UTF-8 conversion.

---

## What's Implemented vs. Planned

### Working Now
- [x] libusb initialization and cleanup
- [x] Device discovery by Razer VID (`0x1532`)
- [x] Known device database (17 devices)
- [x] Battery level query via USB protocol
- [x] Transaction ID auto-detection
- [x] CLI modes: `--discover`, `--test-battery`, `--menu`, `--help`, `--version`
- [x] System tray with battery icon (color-coded)
- [x] Tooltip with device name and battery %
- [x] Right-click context menu (device info, refresh, exit)
- [x] Double-click to refresh
- [x] 5-minute auto-refresh timer
- [x] Multi-device discovery and display

### Not Yet Implemented (Phase 1 MVP Remaining)
- [ ] Configuration system (`razer-tray.json` load/save)
- [ ] First-run auto-discovery flow (interactive device selection)
- [ ] Charging status query (disabled, needs debugging)
- [ ] `--list` command (shows configured devices from config)
- [ ] `--verbose` flag for debug output
- [ ] `--config FILE` for alternate config path
- [ ] `--reset-config` to re-run setup

### Future (Phase 2-3, per RESEARCH.md)
- [ ] RGB basic controls (on/off, spectrum, static color)
- [ ] DPI switching (preset stages)
- [ ] Polling rate adjustment
- [ ] Low battery notifications
- [ ] Multi-device tray icons
- [ ] Effect sync across devices
- [ ] Advanced RGB effects
- [ ] Persistence to device memory

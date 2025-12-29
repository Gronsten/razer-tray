# Razer Tray - Architecture Documentation

**Purpose:** Internal technical documentation for understanding how Razer Tray works. Read this BEFORE searching code or asking questions to save time.

**Last Updated:** 2025-12-28 (v1.0.0 initial release)

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Project Structure](#project-structure)
3. [Core Architecture](#core-architecture)
4. [Execution Flow](#execution-flow)
5. [Configuration System](#configuration-system)
6. [Device Monitoring](#device-monitoring)
7. [Icon Rendering](#icon-rendering)
8. [Animation System](#animation-system)
9. [Build System](#build-system)
10. [Function Reference](#function-reference)
11. [Common Development Patterns](#common-development-patterns)
12. [Troubleshooting Guide](#troubleshooting-guide)

---

## Project Overview

**What:** Lightweight Windows system tray application for monitoring Razer Bluetooth device battery levels
**Language:** C++20
**Size:** 501KB executable, ~15MB memory
**Dependencies:** None (native Win32 APIs only)

**Key Technologies:**
- Win32 API (system tray, windows, timers)
- SetupAPI (Bluetooth device enumeration)
- GDI+ (icon rendering)
- Custom JSON parser (no external libraries)

---

## Project Structure

```
razer-tray/
├── src/                          # C++ source code
│   ├── main.cpp                  # Entry point (WinMain)
│   ├── TrayApp.h/cpp             # Main application logic
│   ├── DeviceMonitor.h/cpp       # Bluetooth device enumeration
│   ├── BatteryIcon.h/cpp         # Dynamic icon generation
│   ├── ConfigManager.h/cpp       # JSON config parser
│   ├── SafeHandles.h             # RAII wrappers for Windows handles
│   └── version.h                 # Version constants
│
├── build/                        # CMake build output (gitignored)
│   └── bin/                      # Distributable files
│       ├── RazerTray.exe         # Compiled executable
│       ├── config.json           # Auto-copied runtime config
│       ├── config.example.json   # Auto-copied example config
│       └── razer-config.ps1      # Auto-copied config tool
│
├── config.json                   # Source config (gitignored)
├── config.example.json           # Example config (committed)
├── razer-config.ps1              # Interactive config tool (committed)
├── deploy-local.ps1              # Personal deployment script (gitignored)
│
├── CMakeLists.txt                # Build configuration
├── build.bat                     # Quick build script
├── CHANGELOG.md                  # Release history
├── README.md                     # User-facing documentation
├── CONFIGURATION.md              # Config guide for users
├── DISTRIBUTION.md               # Build/release guide
└── ARCHITECTURE.md               # This file (internal docs)
```

**Git Policy:**
- **Committed:** Source code, example config, docs, build scripts
- **Ignored:** build/, config.json (user settings), deploy-local.ps1 (personal)

---

## Core Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────┐
│                     WinMain (main.cpp)                  │
│                  Entry point, initializes               │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│                 TrayApp (TrayApp.cpp)                   │
│    Main application controller, owns all components     │
│                                                          │
│  ┌────────────────┐  ┌──────────────┐  ┌────────────┐  │
│  │ DeviceMonitor  │  │ BatteryIcon  │  │ ConfigMgr  │  │
│  │ (enumerate BT) │  │ (render icon)│  │ (load JSON)│  │
│  └────────────────┘  └──────────────┘  └────────────┘  │
└─────────────────────────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│              Windows Message Loop                       │
│  Handles: Tray clicks, timers, menu commands            │
└─────────────────────────────────────────────────────────┘
```

### Memory Management Strategy

**RAII Throughout:**
- Smart pointers for heap allocations (`std::unique_ptr`)
- Custom RAII wrappers in `SafeHandles.h` for Windows handles
- No manual `new`/`delete` anywhere
- Automatic cleanup on scope exit

**Example (SafeHandles.h):**
```cpp
class DeviceInfoHandle {
    HDEVINFO handle;
public:
    DeviceInfoHandle(HDEVINFO h) : handle(h) {}
    ~DeviceInfoHandle() { if (isValid()) SetupDiDestroyDeviceInfoList(handle); }
    bool isValid() const { return handle != INVALID_HANDLE_VALUE; }
    HDEVINFO get() const { return handle; }
};
```

---

## Execution Flow

### 1. Startup Sequence

**File:** `main.cpp` (lines 5-14)

```cpp
int WINAPI WinMain(...) {
    TrayApp app(hInstance);
    if (!app.initialize()) return 1;
    app.run();  // Message loop
    return 0;
}
```

### 2. TrayApp Initialization

**File:** `TrayApp.cpp` (lines 44-62)

**Order of operations:**
1. `createWindow()` - Creates hidden message-only window
2. `addTrayIcon()` - Adds icon to system tray
3. `discoverDevices()` - Initial Bluetooth scan
4. `refreshDevices()` - Query battery levels
5. `updateTrayIcon()` - Show battery status
6. `SetTimer(TIMER_REFRESH)` - Start 5-minute auto-refresh

### 3. Configuration Loading

**File:** `TrayApp.cpp` (lines 19-37)

**Fallback strategy:**
```
Try load config.json
  ↓
  ├─ Success → Use loaded config
  │
  └─ Fail → getDefaultConfig()
      ↓
      Try save defaults
      ↓
      Continue with defaults
```

**Default config:**
- `namePatterns: ["BSK*", "Razer*"]`
- `refreshInterval: 300` (5 minutes)
- `batteryThresholds: {high: 60, medium: 30, low: 15}`

### 4. Device Discovery Flow

**File:** `DeviceMonitor.cpp` (lines 42-117)

```
enumerateRazerDevices()
  ↓
SetupDiGetClassDevs("BTHLE") → Get all BT LE devices
  ↓
For each device:
  ├─ Get instance ID (BTHLE\DEV_...)
  ├─ Get friendly name
  ├─ Check if matches config patterns
  │   ├─ namePatterns[] checked first
  │   └─ devices[] array checked second
  ├─ If match → Add to devices list
  └─ Continue
  ↓
Return vector<unique_ptr<RazerDevice>>
```

### 5. Battery Query Flow

**File:** `DeviceMonitor.cpp` (lines 130-161)

```
getBatteryLevel(instanceId)
  ↓
CM_Locate_DevNodeW() → Get device node
  ↓
CM_Get_DevNode_PropertyW(DEVPKEY_Device_BatteryLevel)
  ↓
Return optional<int> (0-100)
```

**Property Key:** `{104EA319-6EE2-4701-BD47-8DDBF425BBE5}` property 2

### 6. Refresh Animation Flow

**File:** `TrayApp.cpp` (lines 199-215, 244-325)

```
refreshDevices() called (double-click/timer/menu)
  ↓
startRefreshAnimation()
  ├─ Set isRefreshing = true
  ├─ Start TIMER_REFRESH_ANIMATION (100ms)
  └─ Tooltip → "Refreshing..."
  ↓
deviceMonitor->updateDeviceInfo() (instant)
GetLocalTime(&lastRefreshTime)
  ↓
SetTimer(TIMER_ANIMATION_STOP, 3000ms)
  ↓
Animation continues for 3 seconds...
  ├─ updateRefreshAnimation() called every 100ms
  └─ Draws spinning white dot on icon
  ↓
TIMER_ANIMATION_STOP fires
  ↓
stopRefreshAnimation()
  ├─ Kill animation timers
  └─ updateTrayIcon() with final battery data
```

### 7. Window Message Handling

**File:** `TrayApp.cpp` (lines 367-405)

**Key messages:**
- `WM_TRAYICON + WM_LBUTTONDBLCLK` → refreshDevices()
- `WM_TRAYICON + WM_RBUTTONUP` → showContextMenu()
- `WM_TIMER + TIMER_REFRESH` → Auto-refresh (every 5 min)
- `WM_TIMER + TIMER_REFRESH_ANIMATION` → Animation frame
- `WM_TIMER + TIMER_ANIMATION_STOP` → Stop animation
- `WM_COMMAND + ID_MENU_REFRESH` → Manual refresh
- `WM_COMMAND + ID_MENU_EXIT` → Quit

---

## Configuration System

### Config Schema

**File:** `config.json` / `config.example.json`

```json
{
  "version": "1.0.0",
  "devices": [
    {
      "name": "BSKV3P 35K",
      "instanceIdPattern": "BTHLE\\DEV_*",
      "enabled": true,
      "description": "Razer Basilisk V3 Pro"
    }
  ],
  "namePatterns": ["BSK*", "Razer*"],
  "refreshInterval": 300,
  "batteryThresholds": {
    "high": 60,
    "medium": 30,
    "low": 15
  }
}
```

### Pattern Matching Logic

**File:** `ConfigManager.cpp` (lines 167-201)

**Two-tier matching:**
1. **namePatterns[]** - Checked first, wildcard support (`*` at end only)
2. **devices[]** - Checked second, explicit device names

**Example:**
- Pattern `"BSK*"` matches "BSKV3P 35K", "BSK MOBILE"
- Explicit device "Razer DeathAdder" matches exact name

### Config File Location

**Priority order:**
1. Same directory as executable: `.\config.json`
2. If missing: Auto-generate defaults
3. Example config: `config.example.json` (reference only)

**Auto-generation:**
- Triggered on first run if no config.json
- Creates default with `BSK*` and `Razer*` patterns
- Saves to disk (fails silently if no permissions)

---

## Device Monitoring

### Bluetooth Enumeration

**File:** `DeviceMonitor.cpp` (lines 45-53)

**API:** `SetupDiGetClassDevs()`

**Parameters:**
- ClassGuid: `nullptr` (all device classes)
- Enumerator: `"BTHLE"` (Bluetooth Low Energy only)
- Flags: `DIGCF_ALLCLASSES | DIGCF_PRESENT` (all classes, currently connected)

**Returns:** Handle to device information set

### Instance ID Format

**Pattern:** `BTHLE\DEV_{MAC_ADDRESS}\{GUID}`

**Example:** `BTHLE\DEV_E0D55E9B23C8\7&12ABC&0&BluetoothLE00000000_DEADBEEF`

**Check:** Lines 93-96 - Must start with `"BTHLE\"`

### Device Properties Queried

**1. Friendly Name (SPDRP_FRIENDLYNAME)**
- **API:** `SetupDiGetDeviceRegistryPropertyW()`
- **Returns:** Device display name (e.g., "BSKV3P 35K")
- **Used for:** Pattern matching, tooltip display

**2. Battery Level (DEVPKEY_Device_BatteryLevel)**
- **API:** `CM_Get_DevNode_PropertyW()`
- **GUID:** `{104EA319-6EE2-4701-BD47-8DDBF425BBE5}` property 2
- **Type:** BYTE (0-100)
- **Returns:** `optional<int>` (nullopt if unavailable)

**3. Connection Status (DEVPKEY_Device_IsConnected)**
- **API:** `CM_Get_DevNode_PropertyW()`
- **GUID:** `{83da6326-97a6-4088-9453-a1923f573b29}` property 15
- **Type:** DEVPROP_BOOLEAN
- **Returns:** true/false

### RazerDevice Structure

**File:** `DeviceMonitor.h` (lines 9-14)

```cpp
struct RazerDevice {
    std::wstring name;              // Friendly name
    std::wstring instanceId;        // Device instance ID
    std::optional<int> batteryLevel; // 0-100 or nullopt
    bool isConnected;               // Connection status
};
```

---

## Icon Rendering

### Battery Icon Design

**File:** `BatteryIcon.cpp`

**Icon size:** 16x16 pixels
**Format:** Standard Windows icon (HICON)

**Visual elements:**
- Battery outline (3px wide, gray)
- Battery terminal (small rectangle on right side)
- Fill color (based on battery level)
- Empty space (white background)

### Color Coding

**File:** `BatteryIcon.cpp` (lines 25-41)

| Range | Color | RGB | Meaning |
|-------|-------|-----|---------|
| 60-100% | Green | (0, 200, 0) | Good |
| 30-59% | Orange | (255, 165, 0) | Medium |
| 15-29% | Red-Orange | (255, 100, 0) | Low |
| 0-14% | Red | (220, 0, 0) | Critical |
| No battery | Gray | (128, 128, 128) | Unknown/disconnected |

### Icon Creation Process

**Function:** `createBatteryIcon(optional<int> level)`

**Steps:**
1. Initialize GDI+ (COM initialization)
2. Create 16x16 bitmap
3. Draw battery outline (rounded rectangle)
4. Draw battery terminal
5. Calculate fill height based on level
6. Fill with color based on thresholds
7. Convert GDI+ Bitmap to HICON
8. Cleanup GDI+ objects
9. Return HICON (caller must DestroyIcon)

**Usage pattern:**
```cpp
HICON icon = batteryIcon->createBatteryIcon(85);  // Green, 85% filled
// ... use icon ...
DestroyIcon(icon);  // Caller responsible for cleanup
```

---

## Animation System

### Refresh Animation

**Purpose:** Visual feedback during battery refresh (even though query is instant)

**Duration:** Exactly 3 seconds
**Frame rate:** 10 FPS (100ms per frame)
**Frames:** 8 positions in full rotation

### Animation Components

**Timers:**
- `TIMER_REFRESH_ANIMATION` (ID: 2) - 100ms interval, draws frames
- `TIMER_ANIMATION_STOP` (ID: 3) - 3000ms one-shot, stops animation

**State:**
- `isRefreshing` (bool) - Animation active flag
- `animationFrame` (int 0-7) - Current frame in rotation

### Animation Drawing

**File:** `TrayApp.cpp` (lines 273-325)

**Algorithm:**
```cpp
// Calculate spinner position (small white dot orbiting icon center)
angle = (animationFrame * 2π) / 8
dotX = centerX + radius * cos(angle)
dotY = centerY + radius * sin(angle)

// Draw 3x3 white dot at position
Ellipse(dotX-1, dotY-1, dotX+2, dotY+2)
```

**Visual:** White dot rotates around battery icon clockwise

### Tooltip During Animation

**Normal:** "Razer Tray\nUpdated: 14:32:15\nBSKV3P 35K: 85%"
**Animating:** "Refreshing..." (simple message)

---

## Build System

### CMake Configuration

**File:** `CMakeLists.txt`

**Key features:**
- Reads version from `src/version.h` automatically
- Sets C++20 standard
- Links Windows libraries (setupapi, cfgmgr32, shell32, gdi32, gdiplus)
- Creates GUI application (no console window)
- Auto-copies runtime files to `build/bin/`

**Version parsing (lines 3-7):**
```cmake
file(STRINGS "src/version.h" VERSION_LINE REGEX "VERSION = \"[0-9]+\\.[0-9]+\\.[0-9]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" PROJECT_VERSION "${VERSION_LINE}")
project(RazerTray VERSION ${PROJECT_VERSION} LANGUAGES CXX)
```

**Auto-copy post-build (lines 58-68):**
```cmake
add_custom_command(TARGET RazerTray POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json" "${CMAKE_BINARY_DIR}/bin/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.example.json" "${CMAKE_BINARY_DIR}/bin/config.example.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/razer-config.ps1" "${CMAKE_BINARY_DIR}/bin/razer-config.ps1"
)
```

### Build Script

**File:** `build.bat`

**Actions:**
1. Create `build/` directory
2. Run CMake with MinGW Makefiles generator
3. Run `mingw32-make`
4. Auto-copy runtime files (via CMake post-build)
5. Display executable size

**Output:** `build/bin/RazerTray.exe` (501KB)

### Linked Libraries

| Library | Purpose |
|---------|---------|
| setupapi | Device enumeration (`SetupDiGetClassDevs`) |
| cfgmgr32 | Device properties (`CM_Get_DevNode_PropertyW`) |
| shell32 | System tray (`Shell_NotifyIconW`) |
| gdi32 | Basic drawing (device contexts) |
| gdiplus | Advanced graphics (icon rendering) |
| ole32 | COM initialization (required by GDI+) |
| comctl32 | Common controls |

---

## Function Reference

### TrayApp.cpp

| Function | Line | Purpose |
|----------|------|---------|
| `TrayApp()` | 9-38 | Constructor, loads config, creates monitors |
| `initialize()` | 44-62 | Create window, tray icon, initial refresh, start timer |
| `createWindow()` | 64-90 | Register and create message-only window |
| `addTrayIcon()` | 92-111 | Add icon to system tray |
| `updateTrayIcon()` | 113-162 | Update icon and tooltip with battery data |
| `showContextMenu()` | 173-189 | Display right-click menu |
| `refreshDevices()` | 199-215 | Trigger battery refresh with animation |
| `getTooltipText()` | 214-237 | Generate tooltip text with devices and timestamp |
| `formatTimestamp()` | 237-242 | Format time as "Updated: HH:MM:SS" |
| `startRefreshAnimation()` | 244-259 | Begin 3-second animation |
| `stopRefreshAnimation()` | 261-271 | End animation and update final icon |
| `updateRefreshAnimation()` | 273-325 | Draw single animation frame |
| `windowProc()` | 350-405 | Windows message handler (static) |

### DeviceMonitor.cpp

| Function | Line | Purpose |
|----------|------|---------|
| `enumerateRazerDevices()` | 42-117 | Scan for BT LE devices matching config |
| `getDeviceNode()` | 119-128 | Convert instance ID to device node |
| `getBatteryLevel()` | 130-161 | Query battery percentage (0-100) |
| `isDeviceConnected()` | 163-190 | Check if device is currently connected |
| `updateDeviceInfo()` | 192-197 | Update battery/connection for all devices |

### ConfigManager.cpp

| Function | Line | Purpose |
|----------|------|---------|
| `loadConfig()` | 6-76 | Load and parse config.json |
| `saveConfig()` | 78-88 | Save config to JSON file |
| `getDefaultConfig()` | 90-101 | Create default config (BSK*, Razer*) |
| `getDefaultConfigPath()` | 103-112 | Get executable directory + config.json |
| `matchesDevicePatterns()` | 167-201 | Check if device name matches config |
| `serializeJson()` | 92-139 | Convert config to JSON string |

### BatteryIcon.cpp

| Function | Line | Purpose |
|----------|------|---------|
| `createBatteryIcon()` | 17-122 | Generate 16x16 battery icon with fill |
| `getBatteryColor()` | 25-41 | Map battery level to RGB color |

---

## Common Development Patterns

### Adding a New Device Property

**Example: Add "DeviceModel" to RazerDevice**

1. **Update struct** (`DeviceMonitor.h`)
   ```cpp
   struct RazerDevice {
       std::wstring name;
       std::wstring instanceId;
       std::optional<int> batteryLevel;
       bool isConnected;
       std::wstring deviceModel;  // NEW
   };
   ```

2. **Query property** (`DeviceMonitor.cpp`)
   ```cpp
   // In enumerateRazerDevices() after getting name
   WCHAR modelBuffer[256] = {};
   if (SetupDiGetDeviceRegistryPropertyW(..., SPDRP_HARDWAREID, ...)) {
       device->deviceModel = std::wstring(modelBuffer);
   }
   ```

3. **Display in tooltip** (`TrayApp.cpp::getTooltipText()`)
   ```cpp
   ss << L"\n" << device->name << L" (" << device->deviceModel << L"): "
      << device->batteryLevel.value() << L"%";
   ```

### Adding a New Timer

**Example: Add periodic config reload**

1. **Add timer ID** (`TrayApp.h`)
   ```cpp
   static constexpr UINT TIMER_CONFIG_RELOAD = 4;
   ```

2. **Start timer** (`TrayApp::initialize()`)
   ```cpp
   SetTimer(hwnd, TIMER_CONFIG_RELOAD, 60000, nullptr);  // 1 minute
   ```

3. **Handle timer** (`TrayApp::windowProc()`)
   ```cpp
   case WM_TIMER:
       if (wParam == TIMER_CONFIG_RELOAD) {
           app->reloadConfig();
       }
       break;
   ```

4. **Cleanup** (`TrayApp::cleanup()`)
   ```cpp
   KillTimer(hwnd, TIMER_CONFIG_RELOAD);
   ```

### Adding a Config Option

**Example: Add "showDisconnected" option**

1. **Update config struct** (`ConfigManager.h`)
   ```cpp
   struct Config {
       std::wstring version;
       std::vector<DeviceConfig> devices;
       std::vector<std::wstring> namePatterns;
       UINT refreshInterval;
       BatteryThresholds batteryThresholds;
       bool showDisconnected;  // NEW
   };
   ```

2. **Parse from JSON** (`ConfigManager.cpp::loadConfig()`)
   ```cpp
   // After parsing batteryThresholds
   if (showDisconnectedPos != std::string::npos) {
       size_t start = content.find(':', showDisconnectedPos) + 1;
       std::string value = content.substr(start, content.find_first_of(",}", start) - start);
       config.showDisconnected = (value.find("true") != std::string::npos);
   }
   ```

3. **Include in defaults** (`ConfigManager.cpp::getDefaultConfig()`)
   ```cpp
   config.showDisconnected = false;
   ```

4. **Serialize to JSON** (`ConfigManager.cpp::serializeJson()`)
   ```cpp
   json << "  \"showDisconnected\": " << (config.showDisconnected ? "true" : "false") << "\n";
   ```

5. **Use in logic** (`TrayApp.cpp::updateTrayIcon()`)
   ```cpp
   for (const auto& device : devices) {
       if (device->isConnected || config->showDisconnected) {
           // Include in tooltip
       }
   }
   ```

---

## Troubleshooting Guide

### Build Issues

**Error: `cannot find -lsetupapi`**
- **Cause:** MinGW libraries not in PATH
- **Fix:** Ensure MinGW bin directory in PATH, reinstall MinGW

**Error: `version.h: No such file or directory`**
- **Cause:** CMake can't find version header
- **Fix:** Verify `src/version.h` exists, rebuild from clean

**Error: `Permission denied` linking RazerTray.exe**
- **Cause:** Executable still running from previous test
- **Fix:** `taskkill /F /IM RazerTray.exe`, then rebuild

### Runtime Issues

**"No devices detected" always shows**
- **Check 1:** Devices are Bluetooth LE (not classic BT or USB)
- **Check 2:** Devices are paired AND connected to Windows
- **Check 3:** Device names match config patterns
- **Debug:** Run `Get-PnpDevice | Where-Object {$_.InstanceId -like "BTHLE*"}`

**Battery level shows as "?" or missing**
- **Cause:** Device doesn't expose battery property
- **Fix:** Not all BT devices support battery reporting
- **Verify:** `Get-PnpDeviceProperty -InstanceId "BTHLE\..." -KeyName "DEVPKEY_Device_BatteryLevel"`

**Config.json not loading**
- **Check 1:** File in same directory as RazerTray.exe
- **Check 2:** Valid JSON syntax (use jsonlint.com)
- **Check 3:** File encoding is UTF-8
- **Fallback:** App uses defaults if load fails

**Tooltip not updating**
- **Check:** Animation completes (3 seconds)
- **Check:** Auto-refresh timer running (5 minutes)
- **Manual:** Double-click icon to force refresh

### Development Issues

**Memory leak detected**
- **Check:** All `createBatteryIcon()` calls followed by `DestroyIcon()`
- **Check:** All RAII handles have proper destructors
- **Tool:** Use MSVC /analyze or Dr. Memory

**Animation jittery**
- **Check:** Timer interval (should be 100ms)
- **Check:** No blocking operations in `updateRefreshAnimation()`
- **Fix:** Move expensive operations out of animation loop

**Config changes not reflected**
- **Cause:** Config loaded once at startup
- **Fix:** Restart application after editing config.json
- **Future:** Add config reload timer or menu option

---

## Notes for Future Development

### Planned Features

**1. Chroma Lighting Control**
- Research: Reverse engineer Razer Chroma SDK for Bluetooth
- Alternative: OpenRazer Windows port
- Challenges: Proprietary protocol, USB/BT differences

**2. USB/2.4GHz Dongle Support**
- Change enumerator from "BTHLE" to also check "USB"
- Different property keys may be needed
- May require HID API instead of SetupAPI

**3. Battery Notifications**
- Add balloon tooltip on low battery
- Check: `NIF_INFO` flag in `Shell_NotifyIconW()`
- Config: Add threshold for notification trigger

**4. Multi-device Icon**
- Show lowest battery of all devices
- Or: Cycle through devices every N seconds
- Or: Composite icon showing multiple batteries

### Code Quality Guidelines

**When adding code:**
- Use RAII for all resource management
- Prefer `std::optional` over nullable pointers
- Use `const` wherever possible
- Document non-obvious algorithms
- Follow existing naming conventions (camelCase for functions, m_prefix for members)

**Testing checklist:**
- Test with 0 devices connected
- Test with 1 device
- Test with multiple devices
- Test config missing, invalid JSON, empty file
- Test with device disconnecting during operation

---

**Document Version:** 1.0.0
**Last Updated:** 2025-12-28
**Maintainer:** Gronsten

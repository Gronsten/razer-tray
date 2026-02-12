# Razer Tray - Research & Planning

**Date**: 2026-01-05
**Purpose**: Complete analysis of openrazer features and planning for Windows razer-tray rebuild

---

## 1. OpenRazer Device Discovery

### How Discovery Works:
1. **udev enumeration** (Linux-specific):
   - Uses `pyudev.Context.list_devices(subsystem='hid')`
   - Scans all HID devices on the system
   - Filters by Razer VID (`0x1532`)

2. **Device matching**:
   - Each device class has `match()` method checking:
     - USB VID:PID
     - Device type file exists in sysfs
     - Interface numbers

3. **Multi-interface handling**:
   - Razer devices expose multiple USB interfaces
   - Driver finds all interfaces for same physical device
   - Groups them under single device object

4. **Serial number retrieval**:
   - Attempts to read serial 3 times (wireless devices flaky)
   - Uses serial as unique device ID
   - Falls back to counter if serial unavailable

### Windows Equivalent:
- **libusb enumeration**: `libusb_get_device_list()`
- **Filter by VID**: `libusb_get_device_descriptor()`
- **Check PID**: Match against known device list
- **Read serial**: `libusb_get_string_descriptor()` with serial index

---

## 2. All OpenRazer Features

### Device Information
- ✅ **Device name** - "Razer Basilisk V3 Pro", etc.
- ✅ **Device type** - mouse, keyboard, mousemat, headset, keypad, accessory, core
- ✅ **Serial number** - Unique device identifier
- ✅ **Firmware version** - e.g., "v1.04"
- ✅ **Driver version** - e.g., "3.11.0"
- ✅ **Keyboard layout** - en_US, de_DE, en_GB, etc. (keyboards only)

### Mouse Features
- ✅ **Battery level** - 0-100% (wireless mice)
- ✅ **Charging status** - Charging/Not charging
- ✅ **Low battery threshold** - Set notification threshold
- ✅ **DPI settings**:
  - Get/Set DPI X/Y independently
  - DPI stages (up to 5 preset levels)
  - Active stage selection
  - Max DPI value (device-specific)
  - Available DPI values
- ✅ **Polling rate**:
  - Get/Set polling rate (125, 500, 1000 Hz)
  - Supported poll rates per device
  - Hyperpolling support (4000/8000 Hz on some models)
- ✅ **Idle time** - Minutes until device enters sleep
- ✅ **Hyperpolling dongle**:
  - Pairing/unpairing
  - LED indicator mode

### RGB Lighting (All Devices)
- ✅ **Brightness** - 0-100 scale
- ✅ **LED zones**:
  - Logo LED
  - Scroll wheel LED
  - Left side LED
  - Right side LED
  - Backlight LED
  - Underglow
  - Per-key (keyboards with matrix)

- ✅ **Effects**:
  - **Static** - Solid color
  - **Breathing** - Pulsing effect (single, dual, or random colors)
  - **Spectrum cycling** - Rainbow cycle
  - **Wave** - Color wave across device (direction configurable)
  - **Reactive** - Lights up on key press/click (speed/color)
  - **Starlight** - Random twinkling (single, dual, or random colors)
  - **Custom frame** - Direct LED control (matrix devices)
  - **Ripple** - Ripple effect from keypress (keyboards)
  - **Fire** - Flame-like effect
  - **None/Off** - All LEDs off

- ✅ **Effect parameters**:
  - Colors (RGB 0-255)
  - Speed/duration
  - Direction (for wave)
  - Randomize colors

- ✅ **Matrix devices** (keyboards):
  - Matrix dimensions (rows x columns)
  - Per-key LED control
  - Custom frame buffer
  - Frame rate control

### Keyboard-Specific
- ✅ **Game mode** - Disable Windows key
- ✅ **Macro keys** - Programmable keys
- ✅ **Key remapping** - Remap any key
- ✅ **Fn key toggle** - Enable/disable Fn layer
- ✅ **Keyswitch optimization** - Adjust actuation (analog keyboards)

### Headset-Specific
- ✅ **Microphone mute LED**
- ✅ **Sidetone volume** - Hear-yourself volume
- ✅ **Equalizer presets**

### Mousemat-Specific
- ✅ **RGB zones** - Multiple lighting zones
- ✅ **Effects** - Same as other devices

### Accessory-Specific
- ✅ **Charging pad** - LED zones for charging states
- ✅ **Mouse dock** - Charging indicator
- ✅ **Laptop stand** - RGB lighting
- ✅ **Addressable RGB controller** - 6 channels for custom RGB strips

### Advanced Features
- ✅ **Effect sync** - Synchronize effects across multiple devices
- ✅ **Persistence** - Save settings to device memory
- ✅ **Screensaver integration** - Turn off LEDs during screensaver
- ✅ **Battery notifications** - Low battery alerts (Linux desktop integration)
- ✅ **Key event management** - Capture key presses for reactive effects

---

## 3. First-Run Auto-Discovery Flow

### Scenario: No config.json present

```
1. Launch razer-tray.exe
   ↓
2. Check for config.json
   ├─ Exists → Load config, skip to main loop
   │
   └─ Missing → Auto-discovery mode
      ↓
3. Scan for Razer devices (libusb)
   ↓
4. Display discovery results:
   "Found 2 Razer device(s):
    [1] Razer Mouse Dock Pro (VID:1532 PID:00A4)
        Serial: PM1234567890
        Features: Battery, RGB, Wireless pairing
    [2] Razer BlackWidow V3 (VID:1532 PID:024E)
        Serial: KB9876543210
        Features: RGB matrix, Macro keys, Game mode
   "
   ↓
5. Interactive selection:
   "Which devices would you like to monitor?
    [1] All devices
    [2] Select specific devices
    [3] Cancel and exit
   "
   ↓
6. If [1] All devices:
   - Enable all found devices
   - Auto-detect features per device

   If [2] Select specific:
   - Prompt for device selection (checkboxes/numbers)
   - Enable only selected devices

   If [3] Cancel:
   - Exit without saving config
   ↓
7. Generate config.json:
   {
     "version": "2.0.0",
     "devices": [
       {
         "vid": "1532",
         "pid": "00a4",
         "serial": "PM1234567890",
         "name": "Razer Mouse Dock Pro",
         "enabled": true,
         "features": {
           "battery": true,
           "rgb": false,
           "dpi": false
         }
       }
     ],
     "refreshInterval": 60,
     "tray": {
       "showBattery": true,
       "showDPI": false
     }
   }
   ↓
8. Save config.json
   ↓
9. Start main application loop with selected devices
```

---

## 4. CLI Switches & Menu Options

### Command Line Switches

```batch
razer-tray.exe [OPTIONS]

Options:
  --discover, -d        Run device discovery and display results
  --list, -l            List currently configured devices
  --config FILE         Use alternate config file
  --reset-config        Delete config and run first-time setup
  --version, -v         Show version information
  --help, -h            Show this help message
  --no-tray             Run in CLI mode without system tray
  --verbose             Enable debug logging
  --test-battery        Test battery query on all devices
  --test-rgb            Test RGB control on all devices
```

### Examples:
```batch
# First time setup
razer-tray.exe --discover

# List devices
razer-tray.exe --list

# Test battery reading
razer-tray.exe --test-battery

# Reset configuration
razer-tray.exe --reset-config
```

### System Tray Menu Options

```
┌─ Razer Tray ─────────────────────┐
│ Razer Mouse Dock Pro             │
│   Battery: 53%                   │
│   DPI: 1600                      │
│   ───────────────────────────────│
│ Razer BlackWidow V3              │
│   Status: Connected              │
│   ───────────────────────────────│
│ ▶ Refresh Devices                │
│ ▶ Configure...                   │
│ ▶ RGB Controls ▶                 │
│   ├─ All Off                     │
│   ├─ Spectrum Cycle              │
│   ├─ Static Color...             │
│   └─ Breathing                   │
│ ▶ DPI Settings ▶                 │
│   ├─ 800 DPI                     │
│   ├─ 1600 DPI ✓                  │
│   ├─ 3200 DPI                    │
│   └─ Custom...                   │
│ ▶ Re-discover Devices            │
│ ───────────────────────────────────│
│ About Razer Tray                 │
│ Exit                             │
└──────────────────────────────────┘
```

---

## 5. User Interface Ideas

### Option A: System Tray Only (Current Approach)
**Pros:**
- Minimal footprint
- Always accessible
- Windows-native feel

**Cons:**
- Limited UI space
- No advanced configuration GUI
- Hard to visualize RGB effects

**Best for:**
- Battery monitoring
- Quick DPI switching
- Basic RGB controls

---

### Option B: Tray + Configuration Window
**Pros:**
- Best of both worlds
- Advanced settings in dedicated window
- Simple monitoring in tray

**Cons:**
- More development work
- Larger executable

**UI Flow:**
```
System Tray (always running)
   ↓ (Right-click → Configure)
Configuration Window
   ├─ Device Tab
   │   └─ Enable/disable devices
   │   └─ View capabilities
   ├─ Battery Tab
   │   └─ Low battery threshold
   │   └─ Notification settings
   ├─ RGB Tab
   │   └─ Effect selection
   │   └─ Color picker
   │   └─ Brightness slider
   │   └─ Sync devices
   └─ Advanced Tab
       └─ Polling rate
       └─ DPI stages
       └─ Refresh interval
```

---

### Option C: CLI Tool + Optional Tray
**Pros:**
- Power users can script it
- Lightweight core
- Tray is optional add-on

**Cons:**
- Less user-friendly for non-technical users

**Example CLI:**
```batch
# Get battery
razer-tray device --serial PM123 battery

# Set DPI
razer-tray device --serial PM123 dpi 1600

# Set RGB effect
razer-tray device --serial PM123 rgb spectrum

# Monitor mode (runs tray)
razer-tray monitor
```

---

### Option D: Web UI (Advanced)
**Pros:**
- Cross-platform UI (works on any browser)
- Rich visualizations
- Mobile access possible

**Cons:**
- Complex architecture (HTTP server)
- Overkill for simple battery monitoring
- Security considerations

**Not recommended** for Windows-only battery tool.

---

## 6. Recommended Architecture

### Hybrid Approach: **System Tray + CLI Switches**

**Core Features:**
1. **Tray icon** - Shows battery status
2. **CLI mode** - For discovery and testing
3. **Context menu** - Basic controls (refresh, RGB presets, DPI)
4. **Config file** - JSON for persistence

**Phase 1 (MVP):**
- ✅ Device discovery (libusb)
- ✅ Battery monitoring
- ✅ System tray icon with battery %
- ✅ Auto-refresh every 5 minutes
- ✅ Config.json auto-generation

**Phase 2:**
- RGB basic controls (on/off, spectrum, static color)
- DPI switching (preset stages)
- Polling rate adjustment
- Low battery notifications

**Phase 3:**
- Multi-device support
- Effect sync
- Advanced RGB (custom colors, effects)
- Persistence to device memory

---

## 7. Implementation Notes

### Windows vs. Linux Differences

| Feature | Linux (openrazer) | Windows (razer-tray) |
|---------|-------------------|----------------------|
| Device discovery | udev (pyudev) | libusb |
| Driver | Kernel module | libusb WinUSB |
| IPC | D-Bus | None (direct control) |
| Permissions | plugdev group | Admin/WinUSB driver |
| Hotplug | udev monitor | libusb hotplug callbacks |
| Config | INI files | JSON |

### Critical Path for Windows:
1. **No kernel driver needed** - libusb uses WinUSB
2. **WinUSB driver** - May need Zadig for first-time setup
3. **Permissions** - Admin not required if WinUSB driver installed correctly
4. **Hotplug** - `libusb_hotplug_register_callback()` for device add/remove

---

## Next Steps

1. ✅ **Clone & analyze openrazer**
2. ✅ **Test USB protocol with libusb**
3. ✅ **Successfully read battery**
4. ⏳ **Create new razer-tray architecture**
5. ⏳ **Implement device discovery**
6. ⏳ **Build core application**
7. ⏳ **Add system tray integration**
8. ⏳ **Implement configuration**

---

**End of Research Document**

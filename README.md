# Razer Tray

A lightweight, native Windows system tray application for monitoring Razer Bluetooth device battery levels.

## Features

- ✅ Real-time battery monitoring for Bluetooth LE Razer mice
- ✅ Battery-shaped system tray icon with color-coded fill level
- ✅ Shows only actually connected devices (not just paired)
- ✅ Auto-refresh every 5 minutes (configurable)
- ✅ Multi-device support
- ✅ **Configurable device patterns** - monitor any Bluetooth LE device
- ✅ **Interactive configuration tool** - checkbox UI to select devices
- ✅ Tiny footprint: **453KB executable, ~15MB memory**
- ✅ No dependencies - runs on any Windows 10+ machine

## Tested Devices

- Razer Basilisk V3 Pro 35K Phantom Green Edition (BSKV3P 35K)
- Razer Basilisk Mobile (BSK MOBILE)

## Battery Icon Color Coding

- **Green** (60-100%): Good battery level
- **Orange** (30-59%): Medium battery level
- **Red-Orange** (15-29%): Low battery
- **Red** (<15%): Critical battery

## Building from Source

### Requirements

- CMake 3.20+
- MinGW-w64 or MSVC compiler
- Windows SDK (for Win32 APIs)

### Build Steps

**Quick Build (Recommended):**
```bash
.\build.bat
```

**Manual Build:**
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"  # or use "Visual Studio 17 2022" for MSVC
mingw32-make  # or msbuild for MSVC
```

The executable will be in `build/bin/RazerTray.exe`

**Note:** CMake automatically copies runtime files (`config.json`, `config.example.json`, `razer-config.ps1`) to `build/bin/` during the build. The `build/bin/` folder is ready to distribute as-is.

## Configuration

### Quick Start with Default Settings

The application works out-of-the-box with default Razer device patterns (`BSK*`, `Razer*`).

**First Run Behavior:**
1. **With `config.json` present** (normal): Uses config file
2. **Without `config.json`** (deleted/missing): Auto-generates default config with Razer patterns
3. **Config.json auto-saved** for next run (if permissions allow)

Just run `RazerTray.exe` and it will automatically detect connected Razer devices.

### Custom Device Configuration

To monitor non-Razer devices or customize which devices to track:

1. **Interactive Configuration (Recommended)**:
   ```powershell
   .\razer-config.ps1
   ```
   - Menu-driven configuration tool
   - Discover and select Bluetooth LE devices
   - Manage Windows startup
   - Automatically updates `config.json`

2. **Manual Configuration**:
   Edit `config.json` in the same directory as the executable:
   ```json
   {
     "version": "1.0.0",
     "devices": [
       {
         "name": "Your Device Name",
         "instanceIdPattern": "BTHLE\\DEV_*",
         "enabled": true,
         "description": "Custom device"
       }
     ],
     "namePatterns": [
       "BSK*",
       "Razer*",
       "YourDevice*"
     ],
     "refreshInterval": 300,
     "batteryThresholds": {
       "high": 60,
       "medium": 30,
       "low": 15
     }
   }
   ```

### Configuration Options

- **`devices`**: Array of specific devices to monitor (name, pattern, enabled/disabled)
- **`namePatterns`**: Wildcard patterns for device names (e.g., `"BSK*"` matches "BSKV3P 35K")
- **`refreshInterval`**: How often to update battery levels (in seconds, default: 300 = 5 minutes)
- **`batteryThresholds`**: Color thresholds for battery icon (high/medium/low percentages)

## Usage

1. Run `RazerTray.exe`
2. The application runs in the system tray (no visible window)
3. Hover over the icon to see connected devices and battery levels
4. Right-click for options:
   - **Refresh Now**: Manually update battery levels
   - **Exit**: Close the application

## Technical Details

### Architecture

Modern C++20 implementation using Win32 APIs:

- **Device Enumeration**: SetupAPI (`SetupDiGetClassDevs`)
- **Battery Queries**: Configuration Manager API (`CM_Get_DevNode_Property`)
- **System Tray**: Shell API (`Shell_NotifyIcon`)
- **Icon Rendering**: GDI+ for battery-shaped icons

### Safety Features

- RAII wrappers for all Windows handles (no memory leaks)
- Smart pointers (`std::unique_ptr`) - no manual `new`/`delete`
- Bounds checking with `std::optional` for nullable values
- Range-based for loops to avoid iterator bugs

### Property Keys Used

- **Battery Level**: `{104EA319-6EE2-4701-BD47-8DDBF425BBE5}` property 2
- **Connection Status**: `{83da6326-97a6-4088-9453-a1923f573b29}` property 3

## Comparison with C# Prototype

| Metric | C# (.NET 10) | C++ (Native) |
|--------|--------------|--------------|
| **Executable Size** | 155MB (self-contained) | **230KB** |
| **Memory Usage** | ~188MB | **~15MB** |
| **Dependencies** | .NET SDK or 155MB runtime | **None** |
| **Startup Time** | ~2-3 seconds | **Instant** |

The C++ version is **673x smaller** and **12x less memory** than the C# self-contained version!

## Project Structure

```
razer-tray/
├── src/
│   ├── main.cpp              # Entry point (WinMain)
│   ├── DeviceMonitor.cpp/h   # Bluetooth device enumeration
│   ├── BatteryIcon.cpp/h     # Icon generation (GDI+)
│   ├── TrayApp.cpp/h         # System tray logic
│   ├── ConfigManager.cpp/h   # JSON config parser
│   └── SafeHandles.h         # RAII wrappers for Windows handles
├── razer-config.ps1          # Interactive configuration tool
├── config.json               # Default device configuration (ships with app)
├── config.example.json       # Comprehensive configuration examples
├── CMakeLists.txt            # Build configuration
├── README.md
└── CONFIGURATION.md          # Detailed configuration guide
```

## Distribution Files

When distributing the application, include these files:

```
RazerTray.exe                 # Main executable (497KB)
config.json                   # Default configuration (required)
config.example.json           # Configuration examples (optional)
razer-config.ps1              # Configuration tool (optional)
README.md                     # User documentation (optional)
```

**Minimum distribution**: Just `RazerTray.exe` + `config.json`
**Recommended distribution**: All files above for best user experience

## Known Limitations

- ❌ Bluetooth LE only (USB/2.4GHz dongle not supported)
- ❌ No RGB/DPI control (Windows Bluetooth GATT doesn't expose these)

## Future Enhancements

- [ ] Add support for USB/2.4GHz dongle mode
- [ ] Reverse engineer Bluetooth GATT characteristics for RGB/DPI control
- [x] Configurable refresh interval ✅ (added in v1.1.0)
- [x] Support for non-Razer devices ✅ (added in v1.1.0)
- [ ] Battery level notifications

## License

This project was created for personal use. Feel free to use and modify as needed.

## Credits

Inspired by [razer-taskbar](https://github.com/sanraith/razer-taskbar) for the battery icon design.

Built with assistance from Claude Code.

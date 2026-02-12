# Razer Tray v1.0.0 - Archived Files

This directory contains legacy files from **razer-tray v1.0.0** (Bluetooth version).

## Why Archived?

Razer-tray v2.0+ uses **USB/libusb** instead of Bluetooth, making these files obsolete for current development.

## What's Here

- **config.json** / **config.example.json** - v1.0 Bluetooth configuration
- **CONFIGURATION.md** - v1.0 configuration guide
- **ARCHITECTURE.md** - v1.0 architecture documentation
- **DISTRIBUTION.md** - v1.0 distribution guide
- **deploy-local.ps1** - v1.0 deployment script
- **razer-config.ps1** - v1.0 PowerShell configuration tool
- **CMakeLists.txt** - v1.0 build system (CMake)
- **OPENRAZER_CONFIG_EXAMPLE.md** - Research reference

## v1.0.0 vs v2.0+

| Feature | v1.0.0 (Bluetooth) | v2.0+ (USB) |
|---------|-------------------|-------------|
| **Connection** | Bluetooth LE | USB (via Mouse Dock Pro) |
| **Library** | Windows Bluetooth APIs | libusb-1.0 |
| **Devices** | Direct Bluetooth mice | Wireless mice via dock |
| **Configuration** | JSON config file | INI config file |
| **Build System** | CMake | MinGW batch script |

## Reference

- **v1.0.0 Release**: https://github.com/Gronsten/razer-tray/releases/tag/v1.0.0
- **Current Version**: See root README.md

---

**Note:** These files are kept for reference only. v2.0+ is the active codebase in `src_v2_usb/`.

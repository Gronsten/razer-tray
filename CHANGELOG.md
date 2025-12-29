# Changelog

All notable changes to Razer Tray will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2025-12-28

### Added
- Initial release of Razer Tray
- Real-time battery monitoring for Bluetooth LE devices
- Battery-shaped system tray icon with color-coded fill levels (Green/Orange/Red)
- 3-second refresh animation with spinning indicator
- Timestamp showing last update time in tooltip
- Interactive configuration menu (`razer-config.ps1`)
  - Device discovery with checkbox selection
  - Windows startup management
  - Pattern-matched devices shown as locked (read-only)
- Configurable device patterns with wildcard support (e.g., `BSK*`, `Razer*`)
- Auto-refresh every 5 minutes (configurable via `config.json`)
- Double-click tray icon to manually refresh
- Right-click context menu with Refresh and Exit options
- JSON configuration system with auto-generation fallback
- CMake build system with automated runtime file deployment
- Comprehensive documentation (README, CONFIGURATION, DISTRIBUTION guides)

### Technical Details
- Native C++20 implementation using Win32 APIs
- Zero external dependencies
- Executable size: 501KB
- Memory footprint: ~15MB
- RAII memory management with smart pointers
- Custom JSON parser (no external libraries)
- GDI+ icon rendering with dynamic battery visualization

### Limitations
- Bluetooth LE only (USB/2.4GHz dongles not supported)
- No RGB/DPI control (Windows Bluetooth GATT limitations)

[Unreleased]: https://github.com/Gronsten/razer-tray/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Gronsten/razer-tray/releases/tag/v1.0.0

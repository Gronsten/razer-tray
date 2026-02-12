# Configuration Guide

## Overview

Razer Tray supports configurable device patterns, allowing you to monitor **any Bluetooth LE device**, not just Razer mice!

## Quick Start

### Option 1: Default Configuration (Razer Devices Only)

No configuration needed! Just run `RazerTray.exe` and it will automatically detect:
- Any device with "BSK" in the name (Razer Basilisk series)
- Any device with "Razer" in the name

### Option 2: Interactive Configuration Tool

Use the PowerShell configuration wizard to select devices:

```powershell
.\razer-config.ps1
```

**Features:**
- 🔍 Automatically scans for all Bluetooth LE devices
- ✅ Checkbox interface for easy selection
- 💾 Automatically updates `config.json`
- 🔗 Shows connection status (Connected vs Paired)
- 🎯 **Highlights devices already matched** by existing patterns
- 🧠 **Smart deduplication** - won't add devices already covered by wildcards
- 🎨 **Color-coded**: Cyan = already matched, White = new device

**Steps:**
1. Run the script
2. Review devices:
   - **Cyan + 🔒**: Already matched by pattern (read-only, cannot uncheck)
   - **Cyan**: Explicitly configured (can be unchecked)
   - **White**: Not yet configured (can be checked)
3. Use **Up/Down arrows** to navigate
4. Press **Spacebar** to select/deselect devices
   - ⚠️ **Pattern-matched devices (🔒) cannot be unchecked** - to remove them, edit `namePatterns` in config.json
5. Press **A** to select all, **N** to deselect all (respects locked devices)
6. Press **Enter** to save configuration
7. Config is saved to `config.json`
8. **First run only**: Asked if you want to add app to Windows startup (Y/N)

## Configuration File Format

### Location

The `config.json` file must be in the **same directory** as `RazerTray.exe`.

### Example Configuration

```json
{
  "version": "1.0.0",
  "devices": [
    {
      "name": "BSKV3P 35K",
      "instanceIdPattern": "BTHLE\\DEV_*",
      "enabled": true,
      "description": "Razer Basilisk V3 Pro 35K Gaming Mouse"
    },
    {
      "name": "MX Master 3S",
      "instanceIdPattern": "BTHLE\\DEV_*",
      "enabled": true,
      "description": "Logitech MX Master 3S"
    }
  ],
  "namePatterns": [
    "BSK*",
    "Razer*",
    "MX Master*"
  ],
  "refreshInterval": 300,
  "batteryThresholds": {
    "high": 60,
    "medium": 30,
    "low": 15
  }
}
```

## How Pattern Matching Works

The application uses a **two-tier matching system**:

1. **First checks `namePatterns`** (wildcards) - e.g., `"BSK*"` matches "BSKV3P 35K", "BSK MOBILE"
2. **Then checks `devices[]`** (explicit) - only for devices NOT matched by patterns

**Example with redundancy:**
```json
{
  "devices": [
    {"name": "BSKV3P 35K", ...},  // ← Redundant! Already matched by "BSK*"
    {"name": "BSK MOBILE", ...}   // ← Redundant! Already matched by "BSK*"
  ],
  "namePatterns": ["BSK*", "Razer*"]  // ← These already cover the devices above!
}
```

**The razer-config.ps1 script automatically removes this redundancy:**

- If you select "BSKV3P 35K" and "BSK MOBILE"
- Script sees they're both matched by existing `"BSK*"` pattern
- Script **won't add them** to `devices[]` array
- Result: Clean config with no duplicates!

**After optimization:**
```json
{
  "devices": [],  // Empty - everything matched by patterns!
  "namePatterns": ["BSK*", "Razer*"]
}
```

This keeps your config clean and makes it easier to add new devices later (just pair a new "BSK" device and it's automatically detected).

## Configuration Options

### `devices` (Array)

List of specific devices to monitor. Each device has:

- **`name`** (string, required): Exact device name or prefix
- **`instanceIdPattern`** (string): Windows device instance ID pattern (usually `"BTHLE\\DEV_*"`)
- **`enabled`** (boolean): Set to `false` to temporarily disable without removing
- **`description`** (string): Human-readable description (for documentation only)

### `namePatterns` (Array of strings)

Wildcard patterns for device name matching. Supports:

- **`*` wildcard**: Matches any characters at the end
  - Example: `"BSK*"` matches "BSKV3P 35K", "BSK MOBILE", etc.
  - Example: `"Razer*"` matches any device starting with "Razer"
- **Exact match**: No wildcard matches exact name only

**Pattern Matching Logic:**
1. First checks if device name matches any `namePatterns`
2. Then checks if device name contains any `devices[].name` (if `enabled: true`)

### `refreshInterval` (Number)

How often to update battery levels, in **seconds**.

- Default: `300` (5 minutes)
- Minimum recommended: `60` (1 minute)
- Maximum: Any value, but longer intervals save battery on the monitored device

### `batteryThresholds` (Object)

Percentage thresholds for battery icon colors:

- **`high`**: Green color (default: 60%)
- **`medium`**: Orange color (default: 30%)
- **`low`**: Red-orange color (default: 15%)

Icon colors:
- 60-100%: Green
- 30-59%: Orange
- 15-29%: Red-orange
- 0-14%: Red

## Finding Device Names

If you don't know the exact name of your Bluetooth device:

### Method 1: Use the Configuration Script

```powershell
.\razer-config.ps1
```

The script will show all Bluetooth LE devices with their exact names.

### Method 2: PowerShell Command

```powershell
Get-PnpDevice -Class 'Bluetooth' -PresentOnly |
    Where-Object { $_.InstanceId -like 'BTHLE\*' } |
    Select-Object Name, InstanceId
```

### Method 3: Windows Settings

1. Open **Settings** → **Bluetooth & devices**
2. Look at the list of paired devices
3. Use the exact name shown there

## Removing Devices from Monitoring

### Devices Explicitly in `devices[]` Array

**Easy**: Use `razer-config.ps1` and uncheck them. They'll be removed from the `devices[]` array.

### Devices Matched by `namePatterns[]` (🔒 Locked)

**Manual edit required**: The razer-config.ps1 script **cannot uncheck** these devices because they're matched by wildcard patterns.

**To stop monitoring pattern-matched devices:**

1. **Option A - Remove the pattern** (stops monitoring ALL matching devices):
   ```json
   // BEFORE
   { "namePatterns": ["BSK*", "Razer*"] }

   // AFTER (stops monitoring all BSK devices)
   { "namePatterns": ["Razer*"] }
   ```

2. **Option B - Make pattern more specific**:
   ```json
   // BEFORE (matches "BSKV3P 35K" and "BSK MOBILE")
   { "namePatterns": ["BSK*"] }

   // AFTER (only matches BSKV3P)
   { "namePatterns": ["BSKV3P*"] }
   ```

3. **Option C - Move to explicit list**:
   ```json
   // Remove from namePatterns
   { "namePatterns": [] }

   // Add only wanted devices explicitly
   { "devices": [
     {"name": "BSKV3P 35K", "enabled": true}
     // BSK MOBILE not listed = not monitored
   ]}
   ```

**Why locked?**
If we allowed unchecking pattern-matched devices, it would be confusing - you'd uncheck it, but it would still be monitored because of the pattern! The lock prevents this confusion.

## Examples

### Monitor All Razer and Logitech Devices

```json
{
  "namePatterns": [
    "Razer*",
    "Logitech*",
    "MX Master*"
  ]
}
```

### Monitor Specific Devices Only

```json
{
  "devices": [
    {
      "name": "BSKV3P 35K",
      "enabled": true
    },
    {
      "name": "MX Anywhere 3",
      "enabled": true
    }
  ],
  "namePatterns": []
}
```

### Temporarily Disable a Device

```json
{
  "devices": [
    {
      "name": "BSKV3P 35K",
      "enabled": true
    },
    {
      "name": "BSK MOBILE",
      "enabled": false  // Won't be monitored
    }
  ]
}
```

### Fast Refresh for Testing

```json
{
  "refreshInterval": 60,  // Update every 1 minute
  "batteryThresholds": {
    "high": 70,    // Green above 70%
    "medium": 40,  // Orange 40-70%
    "low": 20      // Red-orange 20-40%, Red below 20%
  }
}
```

## Troubleshooting

### Device Not Detected

1. **Check if device is connected** (not just paired):
   ```powershell
   Get-PnpDevice -FriendlyName "Your Device Name"
   ```
   Status should be "OK", not "Unknown" or "Error"

2. **Verify device name** matches exactly (case-sensitive):
   - Use `razer-config.ps1` to see exact names

3. **Check Bluetooth LE support**:
   - USB/2.4GHz dongles are **not supported** (Bluetooth LE only)
   - Older Bluetooth mice may not support LE

### Config Not Loading

1. **Check config.json location**:
   - Must be in the **same directory** as `RazerTray.exe`

2. **Validate JSON syntax**:
   - Use an online JSON validator (like [jsonlint.com](https://jsonlint.com))
   - Common errors: Missing commas, trailing commas, unmatched braces

3. **Check file encoding**:
   - Must be UTF-8 (default in most text editors)

### Battery Level Not Showing

Some Bluetooth devices don't expose battery levels via standard Windows APIs. This is a hardware/driver limitation, not a bug in the application.

**Known to work:**
- Razer Basilisk V3 Pro 35K
- Razer Basilisk Mobile
- Most modern Bluetooth LE mice (2020+)

**May not work:**
- Older Bluetooth mice (pre-2018)
- Mice connected via USB or 2.4GHz dongle
- Devices without Bluetooth LE support

## Configuration File Schema

For developers or advanced users, here's the complete schema:

```typescript
interface Config {
  version: string;                    // Semantic version (e.g., "1.0.0")
  devices: DevicePattern[];           // Array of specific devices
  namePatterns: string[];             // Array of wildcard patterns
  refreshInterval: number;            // Seconds between updates
  batteryThresholds: {
    high: number;                     // Percentage (0-100)
    medium: number;                   // Percentage (0-100)
    low: number;                      // Percentage (0-100)
  };
}

interface DevicePattern {
  name: string;                       // Device name or prefix
  instanceIdPattern: string;          // Windows instance ID pattern
  enabled: boolean;                   // Enable/disable without deleting
  description: string;                // Human-readable description
}
```

## Best Practices

1. **Start with the configuration tool**: Run `razer-config.ps1` to generate a working config

2. **Use wildcards for flexibility**: `"Razer*"` is better than listing every Razer model

3. **Keep refresh intervals reasonable**: 5 minutes (300s) is a good balance between freshness and battery drain

4. **Test changes**: After editing config, restart the application to see changes

5. **Backup your config**: Save a copy before making major changes

## Version History

- **v1.1.0**: Added configuration support
  - JSON config file
  - Interactive PowerShell configuration tool
  - Wildcard pattern matching
  - Configurable refresh interval
  - Support for non-Razer devices

- **v1.0.0**: Initial release
  - Hardcoded Razer device detection
  - Fixed 5-minute refresh

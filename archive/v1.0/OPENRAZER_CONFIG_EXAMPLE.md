# OpenRazer Configuration Examples

## Daemon Configuration File
**Location**: `~/.config/openrazer/razer.conf`
**Format**: INI (ConfigParser)

```ini
[General]
# Verbose logging (logs debug messages - lotsa spam)
verbose_logging = False


[Startup]
# Set the sync effects flag to true so any assignment of effects will work across devices
sync_effects_enabled = True

# Turn off the devices when the systems screensaver kicks in
devices_off_on_screensaver = True

# Battery notifier
battery_notifier = True

# Battery notification frequency [s] (0 to disable)
battery_notifier_freq = 600

# Battery notifications appear when device reaches this percentage
battery_notifier_percent = 33

# Apply effects saved to disk when daemon starts
restore_persistence = False

# Enable this if you dual boot with Windows and the effect isn't restored. Requires restore_persistence = True
persistence_dual_boot_quirk = False
```

---

## Device Persistence File
**Location**: `~/.config/openrazer/persistence.conf`
**Format**: INI (ConfigParser)
**Purpose**: Saves per-device settings to restore on daemon restart

### Example: Razer Basilisk V3 Pro (Mouse)

```ini
[RazerBasiliskV3Pro_PM1234567890]
# DPI settings
dpi_x = 1600
dpi_y = 1600

# Polling rate (in Hz)
poll_rate = 1000

# Logo LED zone
logo_active = True
logo_brightness = 50
logo_effect = spectrum
logo_colors =
logo_speed = 2
logo_wave_dir = 1

# Scroll wheel LED zone
scroll_active = True
scroll_brightness = 50
scroll_effect = spectrum
scroll_colors =
scroll_speed = 2
scroll_wave_dir = 1

# Left side LED zone (if present)
left_active = True
left_brightness = 50
left_effect = static
left_colors = 0 255 0
left_speed = 2
left_wave_dir = 1

# Right side LED zone (if present)
right_active = True
right_brightness = 50
right_effect = static
right_colors = 0 255 0
right_speed = 2
right_wave_dir = 1
```

### Example: Razer BlackWidow V3 (Keyboard)

```ini
[RazerBlackWidowV3_KB9876543210]
# Backlight zone (entire keyboard)
backlight_active = True
backlight_brightness = 75
backlight_effect = wave
backlight_colors =
backlight_speed = 2
backlight_wave_dir = 2

# Game mode enabled (disables Windows key)
game_mode = False

# Macro mode
macro_mode = False
```

### Example: Razer Firefly V2 (Mousemat)

```ini
[RazerFireflyV2_MAT123456]
# Matrix zone (LED strip around mat)
matrix_active = True
matrix_brightness = 100
matrix_effect = breathing
matrix_colors = 255 0 0 0 0 255
matrix_speed = 1
matrix_wave_dir = 1
```

---

## Field Meanings

### Per-Device Section Name
- Format: `[{DeviceClassName}_{SerialNumber}]`
- Example: `[RazerBasiliskV3Pro_PM1234567890]`
- Used as storage key to identify device across reboots

### DPI Fields
- `dpi_x` - Horizontal DPI (e.g., 800, 1600, 3200)
- `dpi_y` - Vertical DPI (usually same as X)
- Only saved if device supports DPI control

### Polling Rate
- `poll_rate` - USB polling rate in Hz
- Values: `125`, `500`, `1000`, `4000`, `8000` (device-dependent)

### LED Zone Fields
Each LED zone (logo, scroll, left, right, backlight, etc.) has:

- `{zone}_active` - Boolean (True/False) - Zone enabled
- `{zone}_brightness` - Integer 0-100 - LED brightness
- `{zone}_effect` - String - Active effect name:
  - `static` - Solid color
  - `breathing` - Pulsing effect
  - `spectrum` - Rainbow cycle
  - `wave` - Color wave
  - `reactive` - Lights on keypress/click
  - `starlight` - Random twinkling
  - `none` - LEDs off

- `{zone}_colors` - Space-separated RGB triplets
  - Empty string for effects with no colors (spectrum)
  - Single color: `255 0 0` (red)
  - Dual color: `255 0 0 0 0 255` (red to blue)
  - Triple color: `255 0 0 0 255 0 0 0 255` (red green blue)

- `{zone}_speed` - Integer 1-3 - Animation speed
  - `1` = Slow
  - `2` = Medium
  - `3` = Fast

- `{zone}_wave_dir` - Integer 1-2 - Wave direction
  - `1` = Left to right / top to bottom
  - `2` = Right to left / bottom to top

---

## How OpenRazer Uses These Files

### On Daemon Start:
1. Read `razer.conf` for global settings
2. Read `persistence.conf` for per-device state
3. Enumerate all connected Razer devices
4. For each device:
   - Check if section exists in persistence file
   - If `restore_persistence = True`:
     - Restore DPI, polling rate, LED settings
   - If not found or disabled:
     - Use device defaults

### During Runtime:
- Every 10 seconds, check if device state changed
- If changed, write updated state to `persistence.conf`
- This happens in background thread (`PersistenceAutoSave`)

### On Daemon Shutdown:
- Write final state to `persistence.conf`
- Ensures settings are saved for next boot

---

## Notes

1. **No per-device enable/disable** - OpenRazer manages all detected devices
2. **Section names include serial** - Unique per physical device
3. **INI format** - Simple, human-readable
4. **Auto-save every 10 seconds** - Prevents loss on crash
5. **Hardware DPI button changes NOT tracked** - Only API changes are persisted

---

## Windows razer-tray Comparison

### What We Should Do Differently:

1. **Use JSON instead of INI** - More flexible, nested structures
2. **Add device enable/disable flag** - User choice of which devices to monitor
3. **Separate daemon config from device persistence** - Two files:
   - `config.json` - User preferences, refresh interval, UI settings
   - `devices.json` - Per-device state (auto-generated, updated by daemon)

4. **Track more metadata**:
   - Device name (user-friendly display name)
   - Last seen timestamp
   - Connection status history
   - Battery level history (for graphs)

### Proposed Windows Config Structure:

**config.json**:
```json
{
  "version": "2.0.0",
  "refreshInterval": 60,
  "tray": {
    "showBattery": true,
    "showDPI": true,
    "batteryIcon": "percentage"
  },
  "notifications": {
    "enabled": true,
    "lowBatteryThreshold": 20
  },
  "devices": {
    "enable": ["PM1234567890"],
    "disable": []
  }
}
```

**devices.json** (auto-managed):
```json
{
  "PM1234567890": {
    "vid": "1532",
    "pid": "00a4",
    "name": "Razer Mouse Dock Pro",
    "deviceType": "dock",
    "lastSeen": "2026-01-05T14:30:00Z",
    "state": {
      "dpi": {"x": 1600, "y": 1600},
      "pollRate": 1000,
      "battery": 53,
      "charging": false,
      "zones": {
        "logo": {
          "active": true,
          "brightness": 50,
          "effect": "spectrum",
          "colors": [],
          "speed": 2
        }
      }
    }
  }
}
```

This gives us:
- Clean separation of user settings vs. device state
- Easy to reset config without losing device history
- More readable JSON format
- Extensible for future features

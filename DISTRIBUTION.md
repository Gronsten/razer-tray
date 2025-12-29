# Distribution Guide

## Files to Include

### Minimum Distribution (Works Standalone)

```
RazerTray.exe     # Main executable (497KB)
config.json              # Default configuration
```

**Use case:** Quick deployment, users don't need customization
**Works for:** All Razer devices matching `BSK*` or `Razer*` patterns

---

### Recommended Distribution (Full Experience)

```
RazerTray.exe     # Main executable (497KB)
config.json              # Default configuration
config.example.json      # Comprehensive configuration examples
razer-config.ps1    # Interactive configuration tool
README.md                # User documentation
CONFIGURATION.md         # Detailed configuration guide
```

**Use case:** Users may want to customize or monitor non-Razer devices
**Total size:** ~550KB

---

## Auto-Generation Behavior

If `config.json` is missing, the application will:

1. **Generate default config in memory** with:
   - `namePatterns: ["BSK*", "Razer*"]`
   - `refreshInterval: 300` (5 minutes)
   - `batteryThresholds: {high: 60, medium: 30, low: 15}`

2. **Attempt to save** `config.json` to executable directory
   - Succeeds: Next run loads from file
   - Fails (permissions): Uses in-memory defaults every time

3. **Continue running** with default Razer patterns regardless

**Result:** Application works even without `config.json`, but we recommend shipping it for consistency.

---

## Packaging Scenarios

### Scenario 1: Enterprise Deployment

**Package:**
```
RazerTray.exe
config.json (pre-configured for your devices)
```

**Pre-configure `config.json`:**
```json
{
  "version": "1.0.0",
  "devices": [],
  "namePatterns": ["BSK*", "Razer*", "Logitech*"],  // Add your brands
  "refreshInterval": 600,  // 10 minutes for less network traffic
  "batteryThresholds": {
    "high": 60,
    "medium": 30,
    "low": 15
  }
}
```

**Deployment:**
- Copy to `C:\Program Files\RazerTray\`
- Add to startup folder or registry
- Users get your pre-configured settings
- No PowerShell script needed (IT controls config)

---

### Scenario 2: End User Release (GitHub, etc.)

**Package:**
```
RazerTray-v1.0.0.zip
├── RazerTray.exe
├── config.json
├── config.example.json
├── razer-config.ps1
├── README.md
└── CONFIGURATION.md
```

**Users can:**
- Run exe immediately (works with defaults)
- Run `razer-config.ps1` to customize
- Manually edit `config.json` using examples
- Read documentation for advanced usage

---

### Scenario 3: Portable/USB Drive

**Package:**
```
RazerTray-Portable/
├── RazerTray.exe
├── config.json
└── README.txt  (basic instructions)
```

**Behavior:**
- Config loads from same directory as exe
- Works on any Windows machine with Bluetooth LE
- Config saved in same directory (portable)
- No installation required

---

## File Descriptions

### RazerTray.exe
- **Size:** 497KB
- **Dependencies:** None (pure Win32)
- **Requirements:** Windows 10+ with Bluetooth LE

### config.json
- **Required:** Recommended (app works without it via auto-generation)
- **Purpose:** Default configuration shipped with app
- **Contents:**
  ```json
  {
    "version": "1.0.0",
    "devices": [],
    "namePatterns": ["BSK*", "Razer*"],
    "refreshInterval": 300,
    "batteryThresholds": {"high": 60, "medium": 30, "low": 15}
  }
  ```

### config.example.json
- **Required:** Optional (reference only)
- **Purpose:** Shows all configuration options with comments
- **Use:** Users copy/paste from this when customizing

### razer-config.ps1
- **Required:** Optional (for interactive config)
- **Purpose:** Scans BLE devices, checkbox UI for selection
- **Requirements:** PowerShell 7+ (not included)
- **Use:** Users run this instead of manual JSON editing

### README.md
- **Required:** Optional (user docs)
- **Purpose:** Usage instructions, features, building
- **Format:** Markdown (readable in text editors)

### CONFIGURATION.md
- **Required:** Optional (advanced config docs)
- **Purpose:** Detailed config guide, examples, troubleshooting
- **Format:** Markdown

---

## Version Bumping

When releasing new versions:

1. **Update version in files:**
   - `config.json` → `"version": "1.1.0"`
   - `config.example.json` → `"version": "1.1.0"`
   - `CMakeLists.txt` → `project(RazerTray VERSION 1.1.0)`

2. **Update README.md:**
   - Features list
   - Binary size (if changed)
   - Future Enhancements (mark completed items)

3. **Update CONFIGURATION.md:**
   - Version history at bottom
   - New configuration options
   - Examples for new features

4. **Create release notes** documenting:
   - New features
   - Bug fixes
   - Config changes (if any)
   - Breaking changes (if any)

---

## Build for Distribution

### Windows (MinGW) - Recommended

**Option A: Quick Build (use build script)**
```bash
cd razer-tray
.\build.bat
```

**Option B: Manual Build**
```bash
cd razer-tray
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make
```

Output: `build/bin/RazerTray.exe`

**Note**: CMake automatically copies runtime files (`config.json`, `config.example.json`, `razer-config.ps1`) to `build/bin/` during build. The `build/bin/` folder is ready to distribute as-is.

### Windows (MSVC)

```bash
cd razer-tray
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/RazerTray.exe`

**Note**: For MSVC, you'll need to manually copy runtime files to `build/Release/` (CMake puts executable in different location)

---

## Testing Before Release

### Test Scenarios

1. ✅ **Normal run** - with `config.json` present
2. ✅ **Missing config** - delete `config.json`, verify auto-generation
3. ✅ **Invalid config** - corrupt `config.json`, verify fallback to defaults
4. ✅ **Custom devices** - use `razer-config.ps1` to add non-Razer device
5. ✅ **Read-only directory** - verify app works even if can't save config
6. ✅ **No Bluetooth** - verify app starts (shows "No devices" message)
7. ✅ **Multiple devices** - verify all detected devices shown in tooltip

### Quick Test Commands

```powershell
# Test 1: Normal run
.\RazerTray.exe

# Test 2: Missing config
Remove-Item config.json
.\RazerTray.exe
# Verify config.json was created

# Test 3: Invalid config
echo "invalid json" > config.json
.\RazerTray.exe
# Verify falls back to defaults (check tooltip)

# Test 4: Configure devices
.\razer-config.ps1
# Select devices, verify config updated

# Test 5: Read-only
Set-ItemProperty config.json -Name IsReadOnly -Value $true
.\RazerTray.exe
# Verify still works (uses existing config)
Set-ItemProperty config.json -Name IsReadOnly -Value $false
```

---

## FAQ for Distributors

**Q: Do I need to include PowerShell?**
A: No. PowerShell 7 is only needed for `razer-config.ps1`. Most users can manually edit `config.json` or use defaults.

**Q: What if users delete `config.json`?**
A: App auto-generates defaults and saves new `config.json` (if permissions allow). Users won't notice.

**Q: Can I pre-configure devices for my users?**
A: Yes! Edit `config.json` before distribution. Users get your settings by default.

**Q: Is the app portable?**
A: Yes! Config loads/saves from same directory as exe. No registry, no AppData.

**Q: What's the minimum Windows version?**
A: Windows 10 (for Bluetooth LE APIs). May work on Windows 8.1 but untested.

**Q: Can I rebrand/customize?**
A: Yes, source is available. Change window title, icon, patterns, etc.

**Q: How do I add it to startup?**
A: Copy to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\` or add registry key.

---

## License & Attribution

This project was created for personal use. Feel free to use and modify as needed.

Built with assistance from Claude Code.

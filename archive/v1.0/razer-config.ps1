<#
.SYNOPSIS
Razer Tray Configuration Tool

.DESCRIPTION
Interactive menu-driven tool for configuring Razer Tray:
- Discover and select Bluetooth LE devices to monitor
- Add/remove from Windows startup
- View current configuration

.EXAMPLE
.\razer-config.ps1
#>

#Requires -Version 7

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# Get script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$configPath = Join-Path $scriptDir "config.json"

function Test-DeviceMatchesPattern {
    param(
        [string]$DeviceName,
        [string]$Pattern
    )

    # Wildcard matching (only supports * at end)
    if ($Pattern.EndsWith('*')) {
        $prefix = $Pattern.Substring(0, $Pattern.Length - 1)
        return $DeviceName.StartsWith($prefix)
    }

    # Exact match
    return $DeviceName -eq $Pattern
}

function Get-DeviceMatchReason {
    param(
        [string]$DeviceName,
        [object]$Config
    )

    # Check namePatterns first
    if ($Config.namePatterns) {
        foreach ($pattern in $Config.namePatterns) {
            if (Test-DeviceMatchesPattern -DeviceName $DeviceName -Pattern $pattern) {
                return @{
                    Matched = $true
                    Reason = "Pattern"
                    MatchedBy = $pattern
                }
            }
        }
    }

    # Check devices array
    if ($Config.devices) {
        foreach ($device in $Config.devices) {
            if ($device.enabled -and $DeviceName.Contains($device.name)) {
                return @{
                    Matched = $true
                    Reason = "Explicit"
                    MatchedBy = $device.name
                }
            }
        }
    }

    return @{
        Matched = $false
        Reason = "None"
        MatchedBy = $null
    }
}

function Show-CheckboxSelection {
    param(
        [Parameter(Mandatory=$true)]
        [array]$Items,
        [string]$Title = "Select Items",
        [string]$Instructions = "Use Up/Down arrows, Space to select/deselect, Enter to confirm"
    )

    if ($Items.Count -eq 0) {
        return @()
    }

    $selectedIndexes = @()
    for ($i = 0; $i -lt $Items.Count; $i++) {
        # Pre-select items that are already matched
        $selectedIndexes += $Items[$i].AlreadyMatched
    }

    $currentIndex = 0
    $done = $false

    # Initial draw
    Clear-Host
    Write-Host "`n$Title`n" -ForegroundColor Cyan
    Write-Host $Instructions -ForegroundColor Gray
    Write-Host "Press A to select all, N to deselect all, Q to cancel`n" -ForegroundColor Gray

    $startLine = [Console]::CursorTop

    # Draw items
    for ($i = 0; $i -lt $Items.Count; $i++) {
        $item = $Items[$i]
        $checkbox = if ($selectedIndexes[$i]) { "[x]" } else { "[ ]" }
        $line = "  $checkbox $($item.DisplayText)"

        # Color based on match status
        $color = if ($item.AlreadyMatched) { "DarkGray" } else { "White" }
        Write-Host $line -ForegroundColor $color
    }

    while (-not $done) {
        # Redraw list
        for ($i = 0; $i -lt $Items.Count; $i++) {
            try {
                [Console]::SetCursorPosition(0, $startLine + $i)
            } catch {
                continue
            }

            $item = $Items[$i]
            $checkbox = if ($selectedIndexes[$i]) { "[x]" } else { "[ ]" }
            $arrow = if ($i -eq $currentIndex) { ">" } else { " " }
            $line = "$arrow $checkbox $($item.DisplayText)"

            # Pad to console width to clear previous content
            $line = $line.PadRight([Console]::WindowWidth - 1)

            # Color logic: current item is green, already matched is cyan, normal is white
            if ($i -eq $currentIndex) {
                $color = "Green"
            } elseif ($item.AlreadyMatched) {
                $color = "Cyan"
            } else {
                $color = "White"
            }

            Write-Host $line -ForegroundColor $color -NoNewline
        }

        # Read key
        $key = [Console]::ReadKey($true)

        switch ($key.Key) {
            'UpArrow' {
                if ($currentIndex -gt 0) {
                    $currentIndex--
                }
            }
            'DownArrow' {
                if ($currentIndex -lt ($Items.Count - 1)) {
                    $currentIndex++
                }
            }
            'Spacebar' {
                # Prevent unchecking devices matched by patterns (read-only)
                if ($Items[$currentIndex].MatchedByPattern -and $selectedIndexes[$currentIndex]) {
                    # Show brief flash/feedback that this device is locked
                    [Console]::Beep(800, 100)
                    # Don't toggle - it's read-only
                } else {
                    $selectedIndexes[$currentIndex] = -not $selectedIndexes[$currentIndex]
                }
            }
            'A' {
                for ($i = 0; $i -lt $selectedIndexes.Count; $i++) {
                    $selectedIndexes[$i] = $true
                }
            }
            'N' {
                for ($i = 0; $i -lt $selectedIndexes.Count; $i++) {
                    # Don't deselect pattern-matched devices (read-only)
                    if (-not $Items[$i].MatchedByPattern) {
                        $selectedIndexes[$i] = $false
                    }
                }
            }
            'Enter' {
                $done = $true
            }
            'Q' {
                return @()
            }
        }
    }

    # Return selected items
    $selected = @()
    for ($i = 0; $i -lt $Items.Count; $i++) {
        if ($selectedIndexes[$i]) {
            $selected += $Items[$i]
        }
    }

    return $selected
}

function Get-BluetoothLEDevices {
    param(
        [object]$Config
    )

    Write-Host "`nScanning for Bluetooth LE devices..." -ForegroundColor Cyan

    # Get all BTHLE devices
    $devices = Get-PnpDevice -Class 'Bluetooth' -PresentOnly |
        Where-Object {
            $_.InstanceId -like 'BTHLE\*' -and
            $_.Name -ne 'Bluetooth LE Device' -and
            -not [string]::IsNullOrWhiteSpace($_.Name)
        } |
        Select-Object Name, InstanceId, Status, @{
            Name='IsConnected'
            Expression={$_.Status -eq 'OK'}
        } |
        Sort-Object Name -Unique

    if (-not $devices) {
        Write-Host "  No Bluetooth LE devices found." -ForegroundColor Yellow
        return @()
    }

    Write-Host "  Found $($devices.Count) Bluetooth LE device(s)`n" -ForegroundColor Green

    # Convert to checkbox items with match status
    $items = @()
    foreach ($device in $devices) {
        $matchInfo = Get-DeviceMatchReason -DeviceName $device.Name -Config $Config

        $statusIcon = if ($device.IsConnected) { "🔗" } else { "⏸" }

        # Build display text with match indicator
        $displayText = "$statusIcon $($device.Name)"
        if ($device.IsConnected) {
            $displayText += " (Connected)"
        } else {
            $displayText += " (Paired)"
        }

        if ($matchInfo.Matched) {
            if ($matchInfo.Reason -eq "Pattern") {
                $displayText += " 🔒 [Matched: $($matchInfo.MatchedBy)]"
            } else {
                $displayText += " [Already configured]"
            }
        }

        $items += [PSCustomObject]@{
            Name = $device.Name
            InstanceId = $device.InstanceId
            IsConnected = $device.IsConnected
            AlreadyMatched = $matchInfo.Matched
            MatchReason = $matchInfo.Reason
            MatchedBy = $matchInfo.MatchedBy
            MatchedByPattern = ($matchInfo.Reason -eq "Pattern")  # Read-only if matched by pattern
            DisplayText = $displayText
        }
    }

    return $items
}

function Read-ConfigFile {
    if (-not (Test-Path $configPath)) {
        Write-Host "Config file not found: $configPath" -ForegroundColor Yellow
        return $null
    }

    try {
        $json = Get-Content $configPath -Raw | ConvertFrom-Json
        return $json
    } catch {
        Write-Host "Error reading config file: $_" -ForegroundColor Red
        return $null
    }
}

function Write-ConfigFile {
    param(
        [Parameter(Mandatory=$true)]
        $Config
    )

    try {
        $json = $Config | ConvertTo-Json -Depth 10
        Set-Content -Path $configPath -Value $json -Force
        Write-Host "`n✓ Config saved to: $configPath" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "`n✗ Error saving config: $_" -ForegroundColor Red
        return $false
    }
}

function Add-ToStartup {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ExePath
    )

    try {
        $startupPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
        $appName = "RazerTray"

        # Check if already in startup
        $existing = Get-ItemProperty -Path $startupPath -Name $appName -ErrorAction SilentlyContinue
        if ($existing) {
            Write-Host "  Already in startup: $($existing.$appName)" -ForegroundColor Yellow
            return $true
        }

        # Add to startup
        New-ItemProperty -Path $startupPath -Name $appName -Value $ExePath -PropertyType String -Force | Out-Null
        Write-Host "  ✓ Added to Windows startup" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "  ✗ Failed to add to startup: $_" -ForegroundColor Red
        return $false
    }
}

function Invoke-DiscoverDevices {
    Clear-Host
    Write-Host "╔════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║      Razer Tray - Device Discovery         ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════╝" -ForegroundColor Cyan

    # Check if this is first run (no config.json)
    $isFirstRun = -not (Test-Path $configPath)

    # Load existing config
    $config = Read-ConfigFile
    if (-not $config) {
        # Create default config
        $config = [PSCustomObject]@{
            version = "1.0.0"
            devices = @()
            namePatterns = @("BSK*", "Razer*")
            refreshInterval = 300
            batteryThresholds = [PSCustomObject]@{
                high = 60
                medium = 30
                low = 15
            }
        }
        Write-Host "Created new default configuration" -ForegroundColor Yellow

        # Save default config immediately (before device selection)
        Write-Host "Saving default configuration..." -ForegroundColor Gray
        if (Write-ConfigFile -Config $config) {
            Write-Host "  ✓ Default config saved to: $configPath" -ForegroundColor Green
        } else {
            Write-Host "  ⚠️  Could not save config file (will use in-memory defaults)" -ForegroundColor Yellow
        }
        Write-Host ""
    }

    # Discover devices (with match status)
    $availableDevices = Get-BluetoothLEDevices -Config $config
    if ($availableDevices.Count -eq 0) {
        Write-Host "`nNo Bluetooth LE devices found. Please pair your devices first." -ForegroundColor Yellow
        Write-Host "Press any key to exit..."
        [void][Console]::ReadKey($true)
        return
    }

    # Show current config
    Write-Host "Current Configuration:" -ForegroundColor Cyan
    Write-Host "  Name Patterns: $($config.namePatterns -join ', ')" -ForegroundColor Gray
    if ($config.devices -and $config.devices.Count -gt 0) {
        $explicitDevices = $config.devices | Where-Object { $_.enabled } | ForEach-Object { $_.name }
        if ($explicitDevices.Count -gt 0) {
            Write-Host "  Explicit Devices: $($explicitDevices -join ', ')" -ForegroundColor Gray
        }
    }
    Write-Host ""

    # Show checkbox selection
    Write-Host "Select devices to monitor:" -ForegroundColor Cyan
    Write-Host "  Cyan + 🔒 = Matched by pattern (cannot uncheck - edit config.json to remove)" -ForegroundColor DarkGray
    Write-Host "  Cyan = Explicitly configured (can be unchecked)" -ForegroundColor DarkGray
    Write-Host "  White = Not yet configured (can be checked)" -ForegroundColor DarkGray
    Write-Host ""
    Start-Sleep -Milliseconds 500

    $selectedDevices = Show-CheckboxSelection `
        -Items $availableDevices `
        -Title "═══ Select Devices to Monitor ═══" `
        -Instructions "Use Up/Down arrows, Space to toggle (locked items cannot be unchecked), Enter to save"

    if ($selectedDevices.Count -eq 0) {
        Write-Host "`nNo devices selected. Config not modified." -ForegroundColor Yellow
        Write-Host "Press any key to exit..."
        [void][Console]::ReadKey($true)
        return
    }

    # Smart config update: only add devices not covered by namePatterns
    $newDevices = @()
    $newPatterns = @($config.namePatterns)  # Start with existing patterns

    foreach ($device in $selectedDevices) {
        # Check if device is already covered by a namePattern
        $coveredByPattern = $false
        foreach ($pattern in $newPatterns) {
            if (Test-DeviceMatchesPattern -DeviceName $device.Name -Pattern $pattern) {
                $coveredByPattern = $true
                break
            }
        }

        # If not covered by a pattern, add to devices array
        if (-not $coveredByPattern) {
            $newDevices += [PSCustomObject]@{
                name = $device.Name
                instanceIdPattern = "BTHLE\DEV_*"
                enabled = $true
                description = "User-selected Bluetooth LE device"
            }

            # Optionally add a pattern if device has a common prefix
            if ($device.Name -match '^(\w+)') {
                $prefix = $matches[1]
                $pattern = "$prefix*"
                if ($newPatterns -notcontains $pattern) {
                    # Only suggest, don't auto-add (to avoid pattern explosion)
                    # Users can manually add patterns to config.json if desired
                }
            }
        }
    }

    # Update config
    $config.devices = $newDevices
    # Keep existing namePatterns (don't auto-add new ones to avoid pattern explosion)

    # Save config
    Clear-Host
    Write-Host "╔════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║  Configuration Summary                     ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Selected Devices: $($selectedDevices.Count)" -ForegroundColor Green
    foreach ($device in $selectedDevices) {
        if ($device.AlreadyMatched) {
            Write-Host "  ✓ $($device.Name) (matched by: $($device.MatchedBy))" -ForegroundColor Cyan
        } else {
            Write-Host "  + $($device.Name) (added to config)" -ForegroundColor Green
        }
    }
    Write-Host ""
    Write-Host "Configuration:" -ForegroundColor Cyan
    Write-Host "  Name Patterns: $($config.namePatterns -join ', ')" -ForegroundColor White
    if ($config.devices.Count -gt 0) {
        Write-Host "  Explicit Devices: $($config.devices.name -join ', ')" -ForegroundColor White
    } else {
        Write-Host "  Explicit Devices: None (all devices matched by patterns)" -ForegroundColor DarkGray
    }
    Write-Host "  Refresh Interval: $($config.refreshInterval) seconds" -ForegroundColor White
    Write-Host ""

    if (Write-ConfigFile -Config $config) {
        if ($isFirstRun) {
            Write-Host "Configuration updated successfully!" -ForegroundColor Green
        } else {
            Write-Host "Configuration saved successfully!" -ForegroundColor Green
        }
        Write-Host ""

        # Show tips based on what's in the config
        $hasPatternMatches = $selectedDevices | Where-Object { $_.MatchedByPattern }
        if ($hasPatternMatches) {
            Write-Host "💡 Tip: Devices with 🔒 are matched by name patterns and cannot be removed here." -ForegroundColor Yellow
            Write-Host "   To stop monitoring them, edit config.json and remove/modify the pattern." -ForegroundColor Yellow
            Write-Host "   Example: Remove 'BSK*' from namePatterns to stop monitoring BSK devices." -ForegroundColor Yellow
            Write-Host ""
        }

        Write-Host "ℹ️  Config optimization: Devices already covered by patterns aren't duplicated." -ForegroundColor Cyan
        Write-Host "   Only devices not matched by wildcards are added to the explicit list." -ForegroundColor Cyan
        Write-Host ""
    }

    Write-Host "`nPress any key to return to menu..."
    [void][Console]::ReadKey($true)
}

function Invoke-ManageStartup {
    Clear-Host
    Write-Host "╔════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║      Razer Tray - Startup Management       ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""

    $startupPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
    $appName = "RazerTray"

    # Check current status
    $existing = Get-ItemProperty -Path $startupPath -Name $appName -ErrorAction SilentlyContinue

    Write-Host "Current Status:" -ForegroundColor Cyan
    if ($existing) {
        Write-Host "  ✅ Razer Tray is in Windows startup" -ForegroundColor Green
        Write-Host "  Path: $($existing.$appName)" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Options:" -ForegroundColor Cyan
        Write-Host "  [R] Remove from startup" -ForegroundColor White
        Write-Host "  [U] Update path" -ForegroundColor White
        Write-Host "  [Q] Return to menu" -ForegroundColor White
        Write-Host ""

        $choice = Read-Host "Choose an option"

        switch ($choice.ToUpper()) {
            'R' {
                try {
                    Remove-ItemProperty -Path $startupPath -Name $appName -ErrorAction Stop
                    Write-Host ""
                    Write-Host "✅ Removed from Windows startup" -ForegroundColor Green
                } catch {
                    Write-Host ""
                    Write-Host "❌ Failed to remove: $_" -ForegroundColor Red
                }
            }
            'U' {
                # Find the executable
                $exePath = Join-Path $scriptDir "build\bin\RazerTray.exe"
                if (-not (Test-Path $exePath)) {
                    $exePath = Join-Path $scriptDir "RazerTray.exe"
                }

                if (Test-Path $exePath) {
                    try {
                        Set-ItemProperty -Path $startupPath -Name $appName -Value $exePath
                        Write-Host ""
                        Write-Host "✅ Updated startup path to: $exePath" -ForegroundColor Green
                    } catch {
                        Write-Host ""
                        Write-Host "❌ Failed to update: $_" -ForegroundColor Red
                    }
                } else {
                    Write-Host ""
                    Write-Host "❌ Could not find RazerTray.exe" -ForegroundColor Red
                }
            }
        }
    } else {
        Write-Host "  ⏸ Razer Tray is NOT in Windows startup" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Would you like to add Razer Tray to Windows startup?" -ForegroundColor Cyan
        Write-Host "This will automatically monitor your devices when you log in." -ForegroundColor Gray
        Write-Host ""

        $response = Read-Host "Add to startup? (Y/N)"

        if ($response -eq 'Y' -or $response -eq 'y') {
            # Find the executable
            $exePath = Join-Path $scriptDir "build\bin\RazerTray.exe"
            if (-not (Test-Path $exePath)) {
                $exePath = Join-Path $scriptDir "RazerTray.exe"
            }

            if (Test-Path $exePath) {
                Write-Host ""
                Add-ToStartup -ExePath $exePath
            } else {
                Write-Host ""
                Write-Host "❌ Could not find RazerTray.exe" -ForegroundColor Red
                Write-Host "   Searched in:" -ForegroundColor Gray
                Write-Host "   - $scriptDir\build\bin\RazerTray.exe" -ForegroundColor Gray
                Write-Host "   - $scriptDir\RazerTray.exe" -ForegroundColor Gray
            }
        }
    }

    Write-Host "`nPress any key to return to menu..."
    [void][Console]::ReadKey($true)
}

function Show-MainMenu {
    Clear-Host
    Write-Host "╔════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║         Razer Tray Configuration          ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""

    # Show current config status
    if (Test-Path $configPath) {
        $config = Read-ConfigFile
        if ($config) {
            Write-Host "Current Configuration:" -ForegroundColor Gray
            Write-Host "  Patterns: $($config.namePatterns -join ', ')" -ForegroundColor DarkGray
            if ($config.devices -and $config.devices.Count -gt 0) {
                Write-Host "  Devices: $($config.devices.Count) explicit" -ForegroundColor DarkGray
            }
            Write-Host "  Refresh: $($config.refreshInterval)s" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "⚠️  No configuration found" -ForegroundColor Yellow
    }

    Write-Host ""
    Write-Host "Menu:" -ForegroundColor Cyan
    Write-Host "  [1] Discover & Configure Devices" -ForegroundColor White
    Write-Host "  [2] Manage Startup" -ForegroundColor White
    Write-Host "  [Q] Quit" -ForegroundColor White
    Write-Host ""

    $choice = Read-Host "Choose an option"
    return $choice
}

function Main {
    $continue = $true

    while ($continue) {
        $choice = Show-MainMenu

        switch ($choice) {
            '1' {
                Invoke-DiscoverDevices
            }
            '2' {
                Invoke-ManageStartup
            }
            'Q' {
                $continue = $false
            }
            'q' {
                $continue = $false
            }
            default {
                Write-Host "Invalid option. Press any key to continue..." -ForegroundColor Yellow
                [void][Console]::ReadKey($true)
            }
        }
    }

    Clear-Host
    Write-Host "Thank you for using Razer Tray Configuration!" -ForegroundColor Cyan
}

# Run main function
Main

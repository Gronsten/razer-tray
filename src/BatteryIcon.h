#pragma once

#include <windows.h>
#include <optional>
#include "SafeHandles.h"

class BatteryIcon {
public:
    BatteryIcon();
    ~BatteryIcon();

    // Create a battery-shaped icon with specified level (0-100)
    // Returns HICON that caller is responsible for (use SafeIcon wrapper)
    HICON createBatteryIcon(std::optional<int> batteryLevel);

private:
    static constexpr int ICON_SIZE = 16;  // 16x16 system tray icon

    // Get color based on battery level
    COLORREF getBatteryColor(int batteryLevel);

    // Draw battery shape on device context
    void drawBatteryShape(HDC hdc, int batteryLevel, COLORREF color);

    // GDI+ token for initialization
    ULONG_PTR gdiplusToken;
};

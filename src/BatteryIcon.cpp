#include "BatteryIcon.h"
#include <windows.h>
#include <gdiplus.h>
#include <algorithm>

using namespace Gdiplus;

BatteryIcon::BatteryIcon() : gdiplusToken(0) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

BatteryIcon::~BatteryIcon() {
    // Shutdown GDI+
    if (gdiplusToken != 0) {
        GdiplusShutdown(gdiplusToken);
    }
}

COLORREF BatteryIcon::getBatteryColor(int batteryLevel) {
    if (batteryLevel >= 60) {
        return RGB(0, 200, 0);  // Green
    } else if (batteryLevel >= 30) {
        return RGB(255, 165, 0);  // Orange
    } else if (batteryLevel >= 15) {
        return RGB(255, 100, 0);  // Red-Orange
    } else {
        return RGB(200, 0, 0);  // Red
    }
}

void BatteryIcon::drawBatteryShape(HDC hdc, int batteryLevel, COLORREF color) {
    // Battery dimensions (similar to C# version)
    const int batteryWidth = 10;
    const int batteryHeight = 13;
    const int batteryX = 3;
    const int batteryY = 2;

    // Create pen for outline
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));

    // Create brush for fill
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));

    // Draw battery outline (hollow rectangle)
    SelectObject(hdc, GetStockObject(NULL_BRUSH));  // No fill for outline
    Rectangle(hdc, batteryX, batteryY, batteryX + batteryWidth, batteryY + batteryHeight);

    // Draw battery terminal (small rectangle on top)
    const int terminalWidth = 4;
    const int terminalHeight = 2;
    const int terminalX = batteryX + (batteryWidth - terminalWidth) / 2;
    const int terminalY = batteryY - terminalHeight;
    Rectangle(hdc, terminalX, terminalY, terminalX + terminalWidth, terminalY + terminalHeight);

    // Draw battery fill based on level
    if (batteryLevel > 0) {
        SelectObject(hdc, brush);  // Use solid brush for fill

        int fillHeight = static_cast<int>((batteryHeight - 2) * batteryLevel / 100.0f);
        int fillY = batteryY + batteryHeight - fillHeight - 1;

        Rectangle(hdc,
            batteryX + 1,
            fillY,
            batteryX + batteryWidth - 1,
            batteryY + batteryHeight - 1);
    }

    // Clean up
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

HICON BatteryIcon::createBatteryIcon(std::optional<int> batteryLevel) {
    // Create bitmap for icon
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, ICON_SIZE, ICON_SIZE);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, bitmap));

    // Fill with transparent background
    RECT rect = { 0, 0, ICON_SIZE, ICON_SIZE };
    HBRUSH bgBrush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(memDC, &rect, bgBrush);

    // Determine battery level and color
    int level = batteryLevel.value_or(0);
    COLORREF color = batteryLevel.has_value() ? getBatteryColor(level) : RGB(128, 128, 128);

    // Draw battery shape
    drawBatteryShape(memDC, level, color);

    // Create mask bitmap (for transparency)
    HBITMAP maskBitmap = CreateBitmap(ICON_SIZE, ICON_SIZE, 1, 1, nullptr);
    HDC maskDC = CreateCompatibleDC(screenDC);
    HBITMAP oldMaskBitmap = static_cast<HBITMAP>(SelectObject(maskDC, maskBitmap));

    // Fill mask with white (opaque)
    RECT maskRect = { 0, 0, ICON_SIZE, ICON_SIZE };
    HBRUSH whiteBrush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    FillRect(maskDC, &maskRect, whiteBrush);

    // Create icon from bitmaps
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = maskBitmap;
    iconInfo.hbmColor = bitmap;

    HICON icon = CreateIconIndirect(&iconInfo);

    // Clean up
    SelectObject(memDC, oldBitmap);
    SelectObject(maskDC, oldMaskBitmap);
    DeleteDC(memDC);
    DeleteDC(maskDC);
    DeleteObject(bitmap);
    DeleteObject(maskBitmap);
    ReleaseDC(nullptr, screenDC);

    return icon;
}
